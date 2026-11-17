#include "ClickGui.hpp"
#include "Cbfh.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>
#include <functional>

using namespace geode::prelude;

namespace wmenu {

namespace {

constexpr int kGuiTag = 0x0CBF;

ccColor4F const C_BG     = { 0.055f, 0.067f, 0.086f, 1.f };
ccColor4F const C_ROW    = { 0.086f, 0.102f, 0.133f, 1.f };
ccColor4F const C_ROW_ON = { 0.098f, 0.145f, 0.184f, 1.f };
ccColor4F const C_STROKE = { 0.157f, 0.196f, 0.251f, 1.f };
ccColor4F const C_ACCENT = { 0.000f, 0.898f, 1.000f, 1.f };
ccColor4F const C_TEXT   = { 0.925f, 0.945f, 0.973f, 1.f };
ccColor4F const C_MUTED  = { 0.529f, 0.580f, 0.647f, 1.f };
ccColor4F const C_OFF    = { 0.223f, 0.251f, 0.318f, 1.f };
ccColor4F const C_KNOB   = { 0.970f, 0.980f, 1.000f, 1.f };

struct ModuleDef {
	char const* key;
	char const* name;
	char const* desc;
	bool defaultOn;
};

ModuleDef const kModules[5] = {
	{ "cbfh-enabled",    "CBFH",           "any click becomes a frame perfect jump", false },
	{ "m-frame-counter", "Frame Counter",  "shows the current frame on screen",      false },
	{ "m-dim",           "Dim Background", "dims the game behind this menu",         true  },
	{ "m-sounds",        "UI Sounds",      "sound feedback when you toggle",         true  },
	{ "m-compact",       "Compact Rows",   "tighter rows, smaller panel",            false },
};

ccColor3B c3(ccColor4F c) {
	return ccc3((GLubyte)(c.r * 255.f), (GLubyte)(c.g * 255.f), (GLubyte)(c.b * 255.f));
}

void roundRect(CCDrawNode* node, CCRect rect, float radius, ccColor4F fill) {
	if (!node || rect.size.width <= 0.f || rect.size.height <= 0.f) return;

	float r = std::min(radius, std::min(rect.size.width, rect.size.height) * 0.5f);
	float x = rect.origin.x;
	float y = rect.origin.y;
	float w = rect.size.width;
	float h = rect.size.height;

	node->drawSolidRect({ x + r, y }, { x + w - r, y + h }, fill);
	node->drawSolidRect({ x, y + r }, { x + w, y + h - r }, fill);

	if (r > 0.5f) {
		node->drawDot({ x + r, y + r }, r, fill);
		node->drawDot({ x + w - r, y + r }, r, fill);
		node->drawDot({ x + r, y + h - r }, r, fill);
		node->drawDot({ x + w - r, y + h - r }, r, fill);
	}
}

void setMenusTouchable(CCNode* node, bool touchable) {
	if (!node) return;

	if (auto menu = typeinfo_cast<CCMenu*>(node)) {
		menu->setTouchEnabled(touchable);
	}

	auto children = node->getChildren();
	if (!children) return;

	for (unsigned int i = 0; i < children->count(); ++i) {
		setMenusTouchable(static_cast<CCNode*>(children->objectAtIndex(i)), touchable);
	}
}

void playToggleSound() {
	if (!cfg::getBool("m-sounds", true)) return;
	FMODAudioEngine::sharedEngine()->playEffect("button02.ogg");
}

// Drag slider used for the 1 - 20 frame window.
class FrameSlider : public CCNode, public CCTouchDelegate {
protected:
	float m_width = 200.f;
	int m_min = cbfh::kMinFrames;
	int m_max = cbfh::kMaxFrames;
	int m_value = 3;
	bool m_dragging = false;
	CCDrawNode* m_draw = nullptr;
	std::function<void(int)> m_onChange;

public:
	static FrameSlider* create(float width, int value, std::function<void(int)> onChange) {
		auto slider = new FrameSlider();
		if (slider && slider->initSlider(width, value, std::move(onChange))) {
			slider->autorelease();
			return slider;
		}
		CC_SAFE_DELETE(slider);
		return nullptr;
	}

	bool initSlider(float width, int value, std::function<void(int)> onChange) {
		if (!CCNode::init()) return false;

		m_width = width;
		m_value = std::clamp(value, m_min, m_max);
		m_onChange = std::move(onChange);

		this->setContentSize({ width, 26.f });
		this->setAnchorPoint({ 0.f, 0.f });

		m_draw = CCDrawNode::create();
		this->addChild(m_draw);
		this->redraw();
		return true;
	}

	virtual void onEnter() {
		CCNode::onEnter();
		CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -130, true);
	}

	virtual void onExit() {
		CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
		CCNode::onExit();
	}

	void redraw() {
		if (!m_draw) return;
		m_draw->clear();

		float span = (float)(m_max - m_min);
		float t = span <= 0.f ? 0.f : (float)(m_value - m_min) / span;
		float x = t * m_width;

		roundRect(m_draw, { 0.f, -3.f, m_width, 6.f }, 3.f, C_OFF);
		if (x > 2.f) {
			roundRect(m_draw, { 0.f, -3.f, x, 6.f }, 3.f, C_ACCENT);
		}
		m_draw->drawDot({ x, 0.f }, 8.f, C_BG);
		m_draw->drawDot({ x, 0.f }, 6.f, C_ACCENT);
	}

	void setFromX(float x) {
		float span = (float)(m_max - m_min);
		int value = m_min;
		if (m_width > 0.f && span > 0.f) {
			float t = std::clamp(x / m_width, 0.f, 1.f);
			value = m_min + (int)std::round(t * span);
		}
		if (value == m_value) return;

		m_value = value;
		this->redraw();
		if (m_onChange) m_onChange(m_value);
	}

	virtual bool ccTouchBegan(CCTouch* touch, CCEvent*) {
		auto point = this->convertTouchToNodeSpace(touch);
		if (point.x < -16.f || point.x > m_width + 16.f) return false;
		if (point.y < -20.f || point.y > 20.f) return false;

		m_dragging = true;
		this->setFromX(point.x);
		return true;
	}

	virtual void ccTouchMoved(CCTouch* touch, CCEvent*) {
		if (!m_dragging) return;
		this->setFromX(this->convertTouchToNodeSpace(touch).x);
	}

	virtual void ccTouchEnded(CCTouch*, CCEvent*) {
		m_dragging = false;
	}

	virtual void ccTouchCancelled(CCTouch*, CCEvent*) {
		m_dragging = false;
	}
};

class ClickGui : public CCLayerColor {
protected:
	CCNode* m_panel = nullptr;

	void buildPanel();

public:
	static ClickGui* create() {
		auto gui = new ClickGui();
		if (gui && gui->initWithColor({ 0, 0, 0, 0 })) {
			gui->autorelease();
			gui->setTag(kGuiTag);
			gui->setup();
			return gui;
		}
		CC_SAFE_DELETE(gui);
		return nullptr;
	}

	static ClickGui* find() {
		auto scene = CCDirector::sharedDirector()->getRunningScene();
		if (!scene) return nullptr;
		return typeinfo_cast<ClickGui*>(scene->getChildByTag(kGuiTag));
	}

	virtual void onEnter() {
		CCLayerColor::onEnter();
		this->setTouchEnabled(true);
	}

	void setup() {
		this->setColor({ 0, 0, 0 });
		this->setOpacity(cfg::getBool("m-dim", true) ? 150 : 0);
		this->buildPanel();
	}

	void refresh() {
		if (m_panel) {
			m_panel->removeFromParent();
			m_panel = nullptr;
		}
		this->setOpacity(cfg::getBool("m-dim", true) ? 150 : 0);
		this->buildPanel();
	}

	void closeGui() {
		auto scene = CCDirector::sharedDirector()->getRunningScene();
		this->removeFromParent();
		if (scene) setMenusTouchable(scene, true);
	}

	virtual void registerWithTouchDispatcher() {
		CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
	}

	virtual bool ccTouchBegan(CCTouch*, CCEvent*) {
		// Swallow taps that land on the dim backdrop so the game behind
		// the menu never receives them.
		return true;
	}
	virtual void ccTouchMoved(CCTouch*, CCEvent*) {}
	virtual void ccTouchEnded(CCTouch*, CCEvent*) {}
	virtual void ccTouchCancelled(CCTouch*, CCEvent*) {}

	void onCloseButton(CCObject*) {
		playToggleSound();
		this->closeGui();
	}

	void onToggleRow(CCObject* sender) {
		auto item = static_cast<CCNode*>(sender);
		if (!item) return;

		int index = item->getTag();
		if (index < 0 || index >= 5) return;

		auto const& def = kModules[index];
		bool nowOn = !cfg::getBool(def.key, def.defaultOn);
		cfg::setBool(def.key, nowOn);

		if (index == 0 && !nowOn) {
			cbfh::releaseAll();
		}

		playToggleSound();
		this->refresh();
	}
};

void ClickGui::buildPanel() {
	auto win = CCDirector::sharedDirector()->getWinSize();

	bool compact = cfg::getBool("m-compact", false);
	float rowH = compact ? 32.f : 42.f;
	float gap = 5.f;
	float width = 306.f;
	float headH = 66.f;
	float sliderH = 66.f;
	float footH = 28.f;

	float rowsH = rowH + gap + sliderH + gap + rowH * 4.f + gap * 4.f;
	float height = headH + gap + rowsH + gap + footH;

	auto panel = CCNode::create();
	panel->setAnchorPoint({ 0.f, 0.f });
	panel->setContentSize({ width, height });

	float scale = std::min(1.f, (win.height - 24.f) / height);
	panel->setScale(scale);
	panel->setPosition({
		win.width * 0.5f - width * scale * 0.5f,
		win.height * 0.5f - height * scale * 0.5f,
	});

	// Drop shadow.
	auto shadow = CCDrawNode::create();
	roundRect(shadow, { 3.f, -7.f, width, height }, 18.f, { 0.f, 0.f, 0.f, 0.35f });
	panel->addChild(shadow, -1);

	// Panel body + border + accent line.
	auto body = CCDrawNode::create();
	roundRect(body, { 0.f, 0.f, width, height }, 17.f, C_STROKE);
	roundRect(body, { 1.f, 1.f, width - 2.f, height - 2.f }, 16.f, C_BG);
	body->drawSolidRect({ 16.f, height - 7.f }, { width - 16.f, height - 5.f }, C_ACCENT);
	body->drawSolidRect({ 1.f, height - headH }, { width - 1.f, height - headH + 1.f }, C_STROKE);
	panel->addChild(body, 0);

	auto title = CCLabelBMFont::create("W MENU", "bigFont.fnt");
	title->setAnchorPoint({ 0.f, 0.5f });
	title->setScale(0.62f);
	title->setColor(c3(C_TEXT));
	title->setPosition({ 18.f, height - 28.f });
	panel->addChild(title, 1);

	auto subtitle = CCLabelBMFont::create("click between frames helper", "chatFont.fnt");
	subtitle->setAnchorPoint({ 0.f, 0.5f });
	subtitle->setScale(0.72f);
	subtitle->setColor(c3(C_MUTED));
	subtitle->setPosition({ 18.f, height - 47.f });
	panel->addChild(subtitle, 1);

	// Everything clickable lives in one menu.
	auto menu = CCMenu::create();
	menu->setPosition({ 0.f, 0.f });
	menu->setHandlerPriority(-500);
	panel->addChild(menu, 10);

	// Close button.
	auto closeNode = CCNode::create();
	closeNode->setContentSize({ 24.f, 24.f });
	closeNode->setAnchorPoint({ 0.5f, 0.5f });
	auto closeDraw = CCDrawNode::create();
	roundRect(closeDraw, { -12.f, -12.f, 24.f, 24.f }, 8.f, C_ROW);
	closeNode->addChild(closeDraw);
	auto closeGlyph = CCLabelBMFont::create("x", "bigFont.fnt");
	closeGlyph->setScale(0.55f);
	closeGlyph->setColor(c3(C_MUTED));
	closeNode->addChild(closeGlyph);

	auto closeItem = CCMenuItemSpriteExtra::create(closeNode, this, menu_selector(ClickGui::onCloseButton));
	closeItem->setPosition({ width - 26.f, height - 30.f });
	menu->addChild(closeItem);

	// Module rows.
	float rowX = 14.f;
	float rowW = width - 28.f;
	float row0Top = height - headH - gap;

	auto makeRow = [&](int index, float top) {
		auto const& def = kModules[index];
		bool on = cfg::getBool(def.key, def.defaultOn);

		auto holder = CCNode::create();
		holder->setContentSize({ rowW, rowH });
		holder->setAnchorPoint({ 0.5f, 0.5f });

		float ox = -rowW * 0.5f;
		float oy = -rowH * 0.5f;

		auto draw = CCDrawNode::create();
		roundRect(draw, { ox, oy, rowW, rowH }, 10.f, on ? C_ROW_ON : C_ROW);
		if (on) {
			draw->drawSolidRect({ ox + 5.f, oy + 7.f }, { ox + 7.f, oy + rowH - 7.f }, C_ACCENT);
		}
		holder->addChild(draw);

		auto name = CCLabelBMFont::create(def.name, "bigFont.fnt");
		name->setAnchorPoint({ 0.f, 0.5f });
		name->setScale(compact ? 0.42f : 0.46f);
		name->setColor(on ? c3(C_TEXT) : c3(C_MUTED));
		name->setPosition({ ox + 16.f, oy + (compact ? rowH * 0.5f : rowH * 0.62f) });
		holder->addChild(name);

		if (!compact) {
			auto desc = CCLabelBMFont::create(def.desc, "chatFont.fnt");
			desc->setAnchorPoint({ 0.f, 0.5f });
			desc->setScale(0.62f);
			desc->setColor(c3(C_MUTED));
			desc->setPosition({ ox + 16.f, oy + rowH * 0.29f });
			holder->addChild(desc);
		}

		float pillW = 34.f;
		float pillH = 17.f;
		float pillX = ox + rowW - 16.f - pillW * 0.5f;
		float pillY = oy + rowH * 0.5f;

		auto pill = CCDrawNode::create();
		roundRect(pill, { pillX - pillW * 0.5f, pillY - pillH * 0.5f, pillW, pillH }, pillH * 0.5f,
			on ? C_ACCENT : C_OFF);
		pill->drawDot({ pillX + (on ? 8.5f : -8.5f), pillY }, 6.f, C_KNOB);
		holder->addChild(pill);

		auto item = CCMenuItemSpriteExtra::create(holder, this, menu_selector(ClickGui::onToggleRow));
		item->setTag(index);
		item->setPosition({ rowX + rowW * 0.5f, top - rowH * 0.5f });
		menu->addChild(item);
	};

	makeRow(0, row0Top);

	// Frame window block, right under CBFH.
	float sliderTop = row0Top - rowH - gap;
	float sliderY = sliderTop - sliderH;

	auto block = CCDrawNode::create();
	roundRect(block, { rowX, sliderY, rowW, sliderH }, 10.f, C_ROW);
	panel->addChild(block, 2);

	auto caption = CCLabelBMFont::create("FRAME WINDOW", "chatFont.fnt");
	caption->setAnchorPoint({ 0.f, 0.5f });
	caption->setScale(0.68f);
	caption->setColor(c3(C_MUTED));
	caption->setPosition({ rowX + 16.f, sliderTop - 18.f });
	panel->addChild(caption, 3);

	auto valueLabel = CCLabelBMFont::create(
		CCString::createWithFormat("%d FR", cbfh::windowFrames())->getCString(), "bigFont.fnt");
	valueLabel->setAnchorPoint({ 1.f, 0.5f });
	valueLabel->setScale(0.42f);
	valueLabel->setColor(c3(C_ACCENT));
	valueLabel->setPosition({ rowX + rowW - 16.f, sliderTop - 18.f });
	panel->addChild(valueLabel, 3);

	auto hint = CCLabelBMFont::create("1 - 20 frames", "chatFont.fnt");
	hint->setAnchorPoint({ 1.f, 0.5f });
	hint->setScale(0.58f);
	hint->setColor(c3(C_OFF));
	hint->setPosition({ rowX + rowW - 16.f, sliderTop - 34.f });
	panel->addChild(hint, 3);

	auto slider = FrameSlider::create(rowW - 34.f, cbfh::windowFrames(), [valueLabel](int value) {
		valueLabel->setString(CCString::createWithFormat("%d FR", value)->getCString());
		cbfh::setWindowFrames(value);
	});
	slider->setPosition({ rowX + 17.f, sliderY + 20.f });
	panel->addChild(slider, 3);

	// Remaining modules.
	float nextTop = sliderY - gap;
	for (int i = 1; i < 5; ++i) {
		makeRow(i, nextTop);
		nextTop -= rowH + gap;
	}

	// Footer.
	auto footerLeft = CCLabelBMFont::create("5 MODULES", "chatFont.fnt");
	footerLeft->setAnchorPoint({ 0.f, 0.f });
	footerLeft->setScale(0.58f);
	footerLeft->setColor(c3(C_OFF));
	footerLeft->setPosition({ rowX + 2.f, 9.f });
	panel->addChild(footerLeft, 1);

	auto footerRight = CCLabelBMFont::create("v1.0.0", "chatFont.fnt");
	footerRight->setAnchorPoint({ 1.f, 0.f });
	footerRight->setScale(0.58f);
	footerRight->setColor(c3(C_OFF));
	footerRight->setPosition({ rowX + rowW - 2.f, 9.f });
	panel->addChild(footerRight, 1);

	this->addChild(panel);
	m_panel = panel;
}

} // namespace

CCMenuItemSpriteExtra* makeWButton(CCObject* target, SEL_MenuHandler handler) {
	float box = 44.f;

	auto holder = CCNode::create();
	holder->setContentSize({ box, box });
	holder->setAnchorPoint({ 0.5f, 0.5f });

	auto draw = CCDrawNode::create();
	roundRect(draw, { -box * 0.5f, -box * 0.5f, box, box }, 12.f, C_ACCENT);
	roundRect(draw, { -box * 0.5f + 2.f, -box * 0.5f + 2.f, box - 4.f, box - 4.f }, 10.f, C_BG);
	holder->addChild(draw);

	auto glyph = CCLabelBMFont::create("W", "bigFont.fnt");
	glyph->setScale(0.85f);
	glyph->setColor(c3(C_ACCENT));
	glyph->setPosition({ 0.f, 0.f });
	holder->addChild(glyph);

	auto item = CCMenuItemSpriteExtra::create(holder, target, handler);
	item->setID("cbfh-w-button"_spr);
	return item;
}

void openGui() {
	auto scene = CCDirector::sharedDirector()->getRunningScene();
	if (!scene) return;

	if (auto existing = ClickGui::find()) {
		existing->closeGui();
		return;
	}

	// Freeze every other menu in the scene so nothing behind the click GUI
	// can be pressed while it is open.
	setMenusTouchable(scene, false);

	playToggleSound();

	auto gui = ClickGui::create();
	if (!gui) {
		setMenusTouchable(scene, true);
		return;
	}

	scene->addChild(gui, 1000);
}

bool isOpen() {
	return ClickGui::find() != nullptr;
}

} // namespace wmenu
