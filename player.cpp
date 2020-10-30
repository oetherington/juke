#include "player.hpp"

#define PARSE_FLAGS \
	(libvlc_media_parse_flag_t)(libvlc_media_parse_local | libvlc_media_fetch_local)
#define VOLUME_STEP 5

static libvlc_instance_t *g_inst = nullptr;

void PlayerGlobalInit()
{
	if (!(g_inst = libvlc_new(0, nullptr)))
		throw "Couldn't initialize media player";
}

void PlayerGlobalDestroy()
{
	libvlc_release(g_inst);
}

Player::Player(const std::string &path)
	: m_media(nullptr)
	, m_player(nullptr)
	, m_volume(100)
	, m_finished(false)
{
	Open(path);
}

Player::~Player()
{
	Close();
}

static void PlayerEndReachedCallback(const libvlc_event_t *const ev, void *player)
{
	Player *p = (Player *)player;
	p->SetFinished(true);
}

#include <unistd.h>
void Player::Open(const std::string &path)
{
	Close();

	m_finished = false;

	if (!(m_media = libvlc_media_new_path(g_inst, path.c_str())))
		throw "Cannot open media file";

	libvlc_media_parse_with_options(m_media, PARSE_FLAGS, 3000);

	if (!(m_player = libvlc_media_player_new_from_media(m_media))) {
		Close();
		throw "Cannot play media file";
	}

	// The media needs to be played in order to get the length
	// libvlc_audio_set_volume(m_player, 0);
	// libvlc_media_player_play(m_player);
	// sleep(2);
	// libvlc_media_player_stop(m_player);
	libvlc_audio_set_volume(m_player, m_volume);

	libvlc_event_manager_t *em = libvlc_media_player_event_manager(m_player);
	libvlc_event_attach(em, libvlc_MediaPlayerEndReached,
			PlayerEndReachedCallback, this);
}

void Player::Close()
{
	if (m_media) {
		libvlc_media_release(m_media);
		m_media = nullptr;
	}

	if (m_player) {
		libvlc_media_player_release(m_player);
		m_player = nullptr;
	}
}

void Player::Play()
{
	if (m_player)
		libvlc_media_player_play(m_player);
}

void Player::Pause()
{
	if (m_player)
		libvlc_media_player_pause(m_player);
}

void Player::Stop()
{
	if (m_player)
		libvlc_media_player_stop(m_player);
}

void Player::VolumeUp()
{
	if (m_volume < 100) {
		m_volume += VOLUME_STEP;
		if (m_volume > 100)
			m_volume = 100;
		libvlc_audio_set_volume(m_player, m_volume);
	}
}

void Player::VolumeDown()
{
	if (m_volume > 0) {
		m_volume -= VOLUME_STEP;
		if (m_volume < 0)
			m_volume = 0;
		libvlc_audio_set_volume(m_player, m_volume);
	}
}

size_t Player::GetLength()
{
	return m_player ? libvlc_media_player_get_length(m_player) / 1000 : 0;
}

size_t Player::GetPosition()
{
	return m_player ? libvlc_media_player_get_time(m_player) : 0;
}

void Player::SetPosition(size_t pos)
{
	libvlc_media_player_set_time(m_player, pos);
}

float Player::GetPercentage()
{
	return m_player ? libvlc_media_player_get_position(m_player) : 0;
}

void Player::SetPercentage(float perc)
{
	libvlc_media_player_set_position(m_player, perc);
}

void Player::SetRate(float rate)
{
	libvlc_media_player_set_rate(m_player, rate);
}

std::string Player::GetMetaString(Meta meta)
{
	if (!m_media)
		throw "Media not loaded";

	libvlc_meta_t m;

	switch (meta) {
	case Meta::Title:	m = libvlc_meta_Title;	break;
	case Meta::Artist:	m = libvlc_meta_Artist;	break;
	case Meta::Album:	m = libvlc_meta_Album;	break;
	default:			return "";
	}

	const char *const s = libvlc_media_get_meta(m_media, m);
	return std::string(s ? s : "");
}

unsigned Player::GetTrackNumber()
{
	if (!m_media)
		throw "Media not loaded";

	const char *const tn = libvlc_media_get_meta(m_media, libvlc_meta_TrackNumber);
	return tn ? std::strtoul(tn, nullptr, 10) : 0;
}
