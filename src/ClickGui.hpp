#pragma once

#include <Geode/Geode.hpp>

namespace wmenu {
	// The round W icon that opens the click GUI.
	cocos2d::CCMenuItemSpriteExtra* makeWButton(cocos2d::CCObject* target, cocos2d::SEL_MenuHandler handler);

	// Opens (or closes, when it is already open) the click GUI on the running scene.
	void openGui();

	bool isOpen();
}
