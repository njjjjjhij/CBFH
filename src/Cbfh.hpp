#pragma once

#include <Geode/Geode.hpp>

// Saved-value helpers (Mod saved values, persisted by Geode).
namespace cfg {
	bool getBool(std::string const& key, bool def);
	void setBool(std::string const& key, bool val);
	int getInt(std::string const& key, int def);
	void setInt(std::string const& key, int val);
}

// The CBFH click engine.
//
// A human click arrives at an arbitrary moment, usually *between* two
// simulated frames. CBFH catches that click, holds it as a pending action and
// then applies it exactly on a frame boundary inside PlayLayer::update, right
// before physics runs. The button stays held for the configured window
// (1 - 20 frames), so the engine fires the jump on the earliest frame it is
// able to, which is the frame-perfect click. The click is always applied, the
// player's state is never checked, so it fires even when it would kill you.
namespace cbfh {
	constexpr int kMinFrames = 1;
	constexpr int kMaxFrames = 20;

	bool enabled();
	int windowFrames();
	void setWindowFrames(int frames);
	bool frameCounter();

	// Returns true when the mod took ownership of the button event.
	bool captureClick(PlayerObject* player);
	bool captureRelease(PlayerObject* player);

	// Called once per frame, before the original PlayLayer::update.
	void tick(PlayLayer* layer);

	void releaseAll();
}
