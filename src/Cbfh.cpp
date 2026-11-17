#include "Cbfh.hpp"

using namespace geode::prelude;

namespace cfg {
	bool getBool(std::string const& key, bool def) {
		return Mod::get()->getSavedValue<bool>(key, def);
	}

	void setBool(std::string const& key, bool val) {
		Mod::get()->setSavedValue<bool>(key, val);
	}

	int getInt(std::string const& key, int def) {
		return Mod::get()->getSavedValue<int>(key, def);
	}

	void setInt(std::string const& key, int val) {
		Mod::get()->setSavedValue<int>(key, val);
	}
}

namespace {

struct PendingClick {
	bool player2 = false;
	int remaining = 0;
};

std::vector<PendingClick> g_pending;

// Set while the mod itself drives the buttons, so the hooks let those
// synthetic press / release events through instead of capturing them again.
bool g_internalCall = false;

struct InternalCall {
	InternalCall() { g_internalCall = true; }
	~InternalCall() { g_internalCall = false; }
};

PlayLayer* playingLayer() {
	auto layer = PlayLayer::get();
	if (!layer || layer->m_isPaused) return nullptr;
	return layer;
}

} // namespace

namespace cbfh {

bool enabled() {
	return cfg::getBool("cbfh-enabled", false);
}

bool frameCounter() {
	return cfg::getBool("m-frame-counter", false);
}

int windowFrames() {
	int frames = cfg::getInt("cbfh-frames", 3);
	if (frames < kMinFrames) frames = kMinFrames;
	if (frames > kMaxFrames) frames = kMaxFrames;
	return frames;
}

void setWindowFrames(int frames) {
	if (frames < kMinFrames) frames = kMinFrames;
	if (frames > kMaxFrames) frames = kMaxFrames;
	cfg::setInt("cbfh-frames", frames);
}

bool captureClick(PlayerObject* player) {
	if (g_internalCall) return false;
	if (!enabled()) return false;

	auto layer = playingLayer();
	if (!layer || !player) return false;

	bool player2 = (layer->m_player2 == player);

	// Already holding for this player: just refresh the window instead of
	// stacking a second hold on top of it.
	for (auto& click : g_pending) {
		if (click.player2 == player2) {
			click.remaining = windowFrames();
			return true;
		}
	}

	g_pending.push_back({ player2, windowFrames() });
	return true;
}

bool captureRelease(PlayerObject* player) {
	if (g_internalCall) return false;
	if (!enabled()) return false;

	auto layer = playingLayer();
	if (!layer || !player) return false;

	bool player2 = (layer->m_player2 == player);

	// While a CBFH hold is running the mod owns the button timing, so the
	// human release must not cut the window short.
	for (auto& click : g_pending) {
		if (click.player2 == player2) return true;
	}

	return false;
}

void tick(PlayLayer* layer) {
	if (!layer) return;

	for (auto it = g_pending.begin(); it != g_pending.end();) {
		auto player = it->player2 ? layer->m_player2 : layer->m_player1;

		if (!player || !enabled()) {
			if (player) {
				InternalCall guard;
				player->releaseButton(PlayerButton::Jump);
			}
			it = g_pending.erase(it);
			continue;
		}

		{
			InternalCall guard;
			// Frame-perfect press on this exact frame, held for the window.
			player->pushButton(PlayerButton::Jump);
			if (--it->remaining <= 0) {
				player->releaseButton(PlayerButton::Jump);
			}
		}

		if (it->remaining <= 0) {
			it = g_pending.erase(it);
			continue;
		}
		++it;
	}
}

void releaseAll() {
	auto layer = PlayLayer::get();

	for (auto& click : g_pending) {
		if (!layer) break;
		auto player = click.player2 ? layer->m_player2 : layer->m_player1;
		if (player) {
			InternalCall guard;
			player->releaseButton(PlayerButton::Jump);
		}
	}

	g_pending.clear();
}

} // namespace cbfh
