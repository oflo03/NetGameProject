#include "Sound.h"
#include "Common.h"

// ---- Sound ----

FMOD::System* Sound::system = nullptr;

Sound::Sound(const std::string& path, bool loop)
{
	if (loop)
		system->createSound(path.c_str(), FMOD_LOOP_NORMAL, 0, &sound);
	else
		system->createSound(path.c_str(), FMOD_LOOP_OFF, 0, &sound);
	channel = nullptr;
}

Sound::~Sound()
{
	sound->release();
}

void Sound::init()
{
	FMOD::System_Create(&system);
	system->init(32, FMOD_INIT_NORMAL, NULL);
}

void Sound::release()
{
	system->close();
	system->release();
}

void Sound::play()
{
	system->playSound(sound, nullptr, true, &channel);
	volumeSet(volume);
	resume();
}

void Sound::pause()
{
	channel->setPaused(true);
}

void Sound::resume()
{
	channel->setPaused(false);
}

void Sound::stop()
{
	channel->stop();
}

void Sound::volumeSet(double n)
{
	channel->setVolume(n);
}

void Sound::update()
{
	system->update();
}

bool Sound::isPlaying()
{
	bool isPlaying;
	channel->isPlaying(&isPlaying);
	return isPlaying;
}

// ---- SoundManager ----

SoundManager* SoundManager::instance = nullptr;

SoundManager::SoundManager()
{
	Sound::init();
	// SOUND enum 순서와 같아야 함
	sounds.emplace_back(new Sound("resources/BGM_TitleState.WAV", true));
	sounds.emplace_back(new Sound("resources/BGM_PlayState.WAV", true));
	sounds.emplace_back(new Sound("resources/menu_characterselect.WAV", false));
	sounds.emplace_back(new Sound("resources/dodge_roll.WAV", false));
	sounds.emplace_back(new Sound("resources/dodge_leap.WAV", false));
	sounds.emplace_back(new Sound("resources/empty.WAV", false));
	sounds.emplace_back(new Sound("resources/enemy_death.WAV", false));
	sounds.emplace_back(new Sound("resources/enemy_hurt.WAV", false));
	sounds.emplace_back(new Sound("resources/menu_confirm.WAV", false));
	sounds.emplace_back(new Sound("resources/Pistol_reload.WAV", false));
	sounds.emplace_back(new Sound("resources/Pistol_shot.WAV", false));
	sounds.emplace_back(new Sound("resources/Rifle_reload.WAV", false));
	sounds.emplace_back(new Sound("resources/Rifle_shot.WAV", false));
	sounds.emplace_back(new Sound("resources/Shotgun_reload.WAV", false));
	sounds.emplace_back(new Sound("resources/Shotgun_shot.WAV", false));
	sounds.emplace_back(new Sound("resources/Rocketx_reload.WAV", false));
	sounds.emplace_back(new Sound("resources/Rocket_Shot.WAV", false));
	sounds.emplace_back(new Sound("resources/player_death.WAV", false));
	sounds.emplace_back(new Sound("resources/player_hurt.WAV", false));
	sounds.emplace_back(new Sound("resources/Sword_attack.WAV", false));
	sounds.emplace_back(new Sound("resources/main_gameover.WAV", false));
	sounds.emplace_back(new Sound("resources/boss_intro.WAV", false));
	sounds.emplace_back(new Sound("resources/boss_lazor.WAV", false));
	sounds.emplace_back(new Sound("resources/boss_locking.WAV", false));
	sounds.emplace_back(new Sound("resources/boss_lightning.WAV", false));
	sounds.emplace_back(new Sound("resources/boss_charging.WAV", false));
	sounds.emplace_back(new Sound("resources/BGM_boss.WAV", true));
	sounds.emplace_back(new Sound("resources/BGM_Select.WAV", true));
	sounds.emplace_back(new Sound("resources/END.WAV", false));
}

SoundManager::~SoundManager()
{
	sounds.clear();
	Sound::release();
}

SoundManager* SoundManager::getInstance()
{
	if (instance == nullptr)
		instance = new SoundManager;
	return instance;
}

void SoundManager::play(int type)
{
	sounds[type]->play();
}

void SoundManager::pause(int type)
{
	sounds[type]->pause();
}

void SoundManager::resume(int type)
{
	sounds[type]->resume();
}

void SoundManager::stop(int type)
{
	sounds[type]->stop();
}

void SoundManager::volumeSet(double n)
{
	for (auto& s : sounds) {
		s->volumeSet(n);
	}
}

void SoundManager::update()
{
	Sound::update();
}

bool SoundManager::isPlaying(int type)
{
	return sounds[type]->isPlaying();
}
