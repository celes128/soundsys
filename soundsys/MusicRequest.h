#pragma once

namespace sound {

	enum MUSIC_REQUEST_TYPE {
		MUSIC_REQUEST_TYPE_PLAY,
		MUSIC_REQUEST_TYPE_PAUSE,
		MUSIC_REQUEST_TYPE_STOP
	};

	struct MusicRequest {
		MUSIC_REQUEST_TYPE	type;

		const char	*filename;
	};

	static MusicRequest MakeMusicRequest_Play(const char *filename)
	{
		return MusicRequest{ MUSIC_REQUEST_TYPE_PLAY, filename };
	}

	static MusicRequest MakeMusicRequest_Pause()
	{
		return MusicRequest{ MUSIC_REQUEST_TYPE_PAUSE, "" };
	}

	static MusicRequest MakeMusicRequest_Stop()
	{
		return MusicRequest{ MUSIC_REQUEST_TYPE_STOP, "" };
	}
}