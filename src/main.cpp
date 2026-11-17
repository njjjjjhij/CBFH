#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include "ClickGui.hpp"
#include "Cbfh.hpp"

using namespace geode::prelude;

namespace {

void addWButton(CCNode* layer, CCObject* target, SEL_MenuHandler handler) {
	if (!layer) return;

	auto button = wmenu::makeWButton(target, handler);
	if (!button) return;

	button->setPosition({ 34.f, 28.f });

	auto menu = CCMenu::create();
	menu->setPosition({ 0.f, 0.f });
	menu->setZOrder(999);
	menu->setID("cbfh-w-button-menu"_spr);
	menu->addChild(button);
	layer->addChild(menu);
}

} // namespace

// The W icon in the main menu.
class $modify(CbfhMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) return false;
		addWButton(this, this, menu_selector(CbfhMenuLayer::onOpenClickGui));
		return true;
	}

	void onOpenClickGui(CCObject*) {
		wmenu::openGui();
	}
};

// The W icon in the pause menu.
class $modify(CbfhPauseLayer, PauseLayer) {
	bool init() {
		if (!PauseLayer::init()) return false;
		addWButton(this, this, menu_selector(CbfhPauseLayer::onOpenClickGui));
		return true;
	}

	void onOpenClickGui(CCObject*) {
		wmenu::openGui();
	}
};

// Frame counter overlay + the CBFH frame hook.
class $modify(CbfhPlayLayer, PlayLayer) {
	struct Fields {
		CCLabelBMFont* frameLabel = nullptr;
		int frame = 0;
	};

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		auto win = CCDirector::sharedDirector()->getWinSize();

		auto label = CCLabelBMFont::create("FRAME 0", "bigFont.fnt");
		label->setAnchorPoint({ 0.f, 1.f });
		label->setScale(0.3f);
		label->setColor({ 0, 229, 255 });
		label->setPosition({ 8.f, win.height - 42.f });
		label->setZOrder(999);
		label->setVisible(false);
		this->addChild(label);

		m_fields->frameLabel = label;
		return true;
	}

	void update(float dt) {
		// Applied before the original update so the press lands exactly on
		// this frame's physics step.
		cbfh::tick(this);

		PlayLayer::update(dt);

		m_fields->frame++;

		if (m_fields->frameLabel) {
			bool show = cbfh::frameCounter();
			m_fields->frameLabel->setVisible(show);
			if (show) {
				m_fields->frameLabel->setString(
					CCString::createWithFormat("FRAME %d", m_fields->frame)->getCString());
			}
		}
	}
};

// CBFH input takeover.
class $modify(CbfhPlayerObject, PlayerObject) {
	void pushButton(PlayerButton button) {
		if (button == PlayerButton::Jump && cbfh::captureClick(this)) {
			return;
		}
		PlayerObject::pushButton(button);
	}

	void releaseButton(PlayerButton button) {
		if (button == PlayerButton::Jump && cbfh::captureRelease(this)) {
			return;
		}
		PlayerObject::releaseButton(button);
	}
};
