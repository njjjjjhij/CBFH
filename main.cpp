#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

// ------------------------------------------------------------
// Settings / config
// ------------------------------------------------------------
struct CBFHSettings {
    bool enabled = true;
    int frames = 8;          // 1..20
    bool forceJump = true;   // force jump even if click would normally kill
};

static CBFHSettings g_cbfh;

// ------------------------------------------------------------
// Simple mobile-style popup GUI
// ------------------------------------------------------------
class CBFHClickGui : public geode::Popup<> {
protected:
    CCLabelBMFont* m_statusLabel = nullptr;
    CCLabelBMFont* m_frameLabel = nullptr;
    CCMenuItemToggler* m_enableToggle = nullptr;
    CCSliderBar* m_frameSlider = nullptr;

    int m_sliderValue = 8;

public:
    static CBFHClickGui* create() {
        auto p = new CBFHClickGui();
        if (p && p->init(200, 220)) {
            p->autorelease();
            return p;
        }
        CC_SAFE_DELETE(p);
        return nullptr;
    }

    bool init(float width, float height) {
        if (!Popup::init(0.0f, 0.0f, width, height)) return false;

        auto bg = CCScale9Sprite::create("square02b_001.png");
        bg->setContentSize({width, height});
        bg->setPosition({0, 0});
        this->m_mainLayer->addChild(bg);

        auto title = CCLabelBMFont::create("CBFH", "bigFont.fnt");
        title->setPosition({0, height * 0.35f});
        this->m_mainLayer->addChild(title);

        // Enabled toggle
        auto toggleBtn = CCMenuItemToggler::create(
            this,
            menu_selector(CBFHClickGui::onToggle),
            "GJ_checkOn_001.png",
            "GJ_checkOff_001.png"
        );
        toggleBtn->setScale(0.8f);
        toggleBtn->setPosition({-60, 40});
        this->m_mainLayer->addChild(toggleBtn);

        auto enableText = CCLabelBMFont::create("Enabled", "goldFont.fnt");
        enableText->setPosition({35, 40});
        this->m_mainLayer->addChild(enableText);

        // Slider label
        m_frameLabel = CCLabelBMFont::create("Frames: 8", "bigFont.fnt");
        m_frameLabel->setPosition({0, 0});
        this->m_mainLayer->addChild(m_frameLabel);

        // Slider
        m_frameSlider = CCSliderBar::create();
        m_frameSlider->setPosition({0, -35});
        m_frameSlider->setMin(1);
        m_frameSlider->setMax(20);
        m_frameSlider->setValue(g_cbfh.frames);
        m_frameSlider->setTarget(this, menu_selector(CBFHClickGui::onSliderChange));
        this->m_mainLayer->addChild(m_frameSlider);

        // Close button
        auto closeBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_arrow_02.png"),
            this,
            menu_selector(CBFHClickGui::onClose)
        );
        closeBtn->setPosition({width * 0.38f, height * 0.38f});
        this->m_mainLayer->addChild(closeBtn);

        this->updateUI();
        return true;
    }

    void updateUI() {
        if (m_frameLabel) {
            m_frameLabel->setString(CCString::createWithFormat("Frames: %d", g_cbfh.frames)->getCString());
        }
        if (m_enableToggle) {
            m_enableToggle->toggle(g_cbfh.enabled);
        }
    }

    void onToggle(CCObject*) {
        g_cbfh.enabled = !g_cbfh.enabled;
        updateUI();
    }

    void onSliderChange(CCObject* sender) {
        auto slider = static_cast<CCSliderBar*>(sender);
        g_cbfh.frames = std::clamp((int)std::round(slider->getValue()), 1, 20);
        updateUI();
    }

    void onClose(CCObject*) {
        this->setVisible(false);
        this->removeFromParentAndCleanup(true);
    }
};

// ------------------------------------------------------------
// Helper: "perfect frame click" scheduler
// ------------------------------------------------------------
static void schedulePerfectJump(float frameOffsetSeconds) {
    // This is just a general cocos scheduler pattern.
    // In actual GD, you often schedule against the current layer/scene.
    // The idea is: after a delay of frameOffsetSeconds, trigger a jump.
    CCDirector::sharedDirector()->getScheduler()->schedule(
        [=](float dt) {
            // This is where the actual jump is triggered.
            // If the game is in a valid PlayLayer / player object, we call jump.
            auto game = GameManager::sharedState();
            auto layer = PlayLayer::get();
            if (!layer) return;

            auto player = layer->m_player1;
            if (!player) return;

            // Force jump/click behavior regardless of screen location:
            // This is the "CBFH" part.
            player->activateGroundJump();
            // optional: player->jump(); // if your GD version uses direct jump API
        },
        CCDirector::sharedDirector(),
        0.0f,
        0,
        frameOffsetSeconds,
        false,
        "cbfh-perfect-jump"
    );
}

// So we can trigger it from click events:
static void triggerCBFHJump() {
    if (!g_cbfh.enabled) return;

    // 1 frame ≈ 1/60 sec
    float seconds = (float)g_cbfh.frames / 60.0f;
    schedulePerfectJump(seconds);
}

// ------------------------------------------------------------
// GUI button in main menu
// ------------------------------------------------------------
class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("main-menu");
        if (!menu) return true;

        auto wIcon = CCSprite::createWithSpriteFrameName("GJ_arrow_01.png");
        auto wBtn = CCMenuItemSpriteExtra::create(wIcon, this, menu_selector(MenuLayer::onWButton));
        wBtn->setPosition({-245, -120});
        wBtn->setScale(0.8f);
        menu->addChild(wBtn);

        return true;
    }

    void onWButton(CCObject*) {
        auto gui = CBFHClickGui::create();
        if (!gui) return;

        auto layer = CCDirector::sharedDirector()->getRunningScene();
        if (layer) {
            layer->addChild(gui);
        }
    }
};

// ------------------------------------------------------------
// GUI button in pause menu
// ------------------------------------------------------------
class $modify(PauseLayer) {
    bool init() {
        if (!PauseLayer::init()) return false;

        auto menu = this->getChildByID("pause-menu");
        if (!menu) return true;

        auto wIcon = CCSprite::createWithSpriteFrameName("GJ_arrow_01.png");
        auto wBtn = CCMenuItemSpriteExtra::create(wIcon, this, menu_selector(PauseLayer::onWButton));
        wBtn->setPosition({-230, 80});
        wBtn->setScale(0.8f);
        menu->addChild(wBtn);

        return true;
    }

    void onWButton(CCObject*) {
        auto gui = CBFHClickGui::create();
        if (!gui) return;
        this->addChild(gui);
    }
};

// ------------------------------------------------------------
// Main CBFH click hook
// ------------------------------------------------------------
class $modify(PlayLayer) {
    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) return false;

        // Hook into actual click/tap behavior if your GD build exposes it
        // Example: m_player1->setInputEnabled(false) or custom input intercept
        return true;
    }

    void onClick(GameObject* obj) {
        PlayLayer::onClick(obj);
        triggerCBFHJump();
    }
};

// ------------------------------------------------------------
// Optional: direct click hook for a mobile input controller
// ------------------------------------------------------------
class $modify(PlayerObject) {
    void controlJump(float dt) {
        PlayerObject::controlJump(dt);

        // If the game sees a tap/click and CBFH is enabled:
        if (g_cbfh.enabled) {
            // Replace ordinary jump with a scheduled “perfect frame” press
            // This is the core of CBFH.
            // Can be triggered on any click anywhere, even if the instant click
            // would normally kill the player.
            triggerCBFHJump();
        }
    }
};

// ------------------------------------------------------------
// Optional Geode mod settings registration
// ------------------------------------------------------------
$execute {
    Mod::get()->registerSetting("cbfh-enabled", &g_cbfh.enabled);
    Mod::get()->registerSetting("cbfh-frames", &g_cbfh.frames);
}
