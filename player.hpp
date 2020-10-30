#pragma once

#include <vlc/vlc.h>
#include <string>

void PlayerGlobalInit();
void PlayerGlobalDestroy();

enum class Meta {
	Title,
	Artist,
	Album,
};

class Player {
private:
	libvlc_media_t *m_media;
	libvlc_media_player_t *m_player;
	int m_volume;
	bool m_finished;

public:
	Player()
		: m_media(nullptr)
		, m_player(nullptr)
		, m_volume(100)
		, m_finished(false)
	{}

	Player(const std::string &path);

	~Player();

	Player(const Player &p) = delete;

	Player(Player &&p)
	{
		m_media = p.m_media;
		m_player = p.m_player;
		p.m_player = nullptr;
		p.m_player = nullptr;
	}

	void Open(const std::string &path);
	void Close();

	void Play();
	void Pause();
	void Stop();

	void VolumeUp();
	void VolumeDown();

	size_t GetLength();
	size_t GetPosition();
	void SetPosition(size_t pos);
	float GetPercentage();
	void SetPercentage(float perc);

	void SetRate(float rate);

	std::string GetMetaString(Meta meta);
	unsigned GetTrackNumber();

	inline bool IsFinished() const { return m_finished; }
	inline void SetFinished(bool val) { m_finished = val; }
};
