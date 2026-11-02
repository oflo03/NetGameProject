#pragma once
#include "fmod.hpp"
#include <string>
#include <vector>
#include <memory>

enum SOUND {
	TitleState,
	MainState,
	BUTTONCLICK,
	WALK,
	ROLLING,
	EMPTY,
	ENEMYDEATH,
	ENEMYHURT,
	CURSORON,
	PISTOL_RELOAD,
	PISTOL_SHOT,
	RIFLE_RELOAD,
	RIFLE_SHOT,
	SHOTGUN_RELOAD,
	SHOTGUN_SHOT,
	ROCKET_RELOAD,
	ROCKET_SHOT,
	PLAYER_DEATH,
	PLAYER_HURT,
	SWORD_ATK,
	GAME_OVER,
	BOSSSCENE,
	BOSSLAZOR,
	BOSSLOCKON,
	BOSSLIGHTNING,
	BOSSCHARGING,
	BOSS,
	SELECTSTATE,
	END
};

class Sound
{
private:
	static FMOD::System* system;
	FMOD::Sound* sound;
	FMOD::Channel* channel;
public:
	Sound(const std::string& path, bool loop);
	~Sound();
	static void init();
	static void release();
	static void update();
	void play();
	void pause();
	void resume();
	void stop();
	void volumeSet(double n);
	bool isPlaying();
};

class SoundManager
{
private:
	static SoundManager* instance;
	std::vector<std::unique_ptr<Sound>> sounds;
	SoundManager();
	~SoundManager();
public:
	static SoundManager* getInstance();
	void play(int type);
	void pause(int type);
	void resume(int type);
	void stop(int type);
	void volumeSet(double n);
	void update();
	bool isPlaying(int type);
};
