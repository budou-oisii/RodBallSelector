#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/LevelPage.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/IDManager.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace {

constexpr int DEFAULT_ROD_BALL = 0;
// These are native Geometry Dash sprite-frame names. GameObject::getBallFrame()
// is the engine helper used by Rod objects to choose their ball texture.
constexpr std::array<char const*, 3> ROD_BALL_FILES = {
    "rod_ball_01_001.png",
    "rod_ball_02_001.png",
    "rod_ball_03_001.png",
};
constexpr std::array<char const*, 3> ROD_BALL_FRAME_NAMES = {
    "rod_ball_01_001.png",
    "rod_ball_02_001.png",
    "rod_ball_03_001.png",
};

// Snapshot the level's saved choice once when PlayLayer starts.
// It stays unchanged for the whole run, so retries / animation updates cannot
// accidentally pick up a different level's value.
int g_activeRodBall = DEFAULT_ROD_BALL;

std::uint64_t stableHash(std::string const& value) {
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char byte : value) {
        hash ^= byte;
        hash *= 1099511628211ull;
    }
    return hash;
}

std::vector<std::string> levelStorageKeys(GJGameLevel* level) {
    if (!level) return { "rod-ball-level-unknown" };

    int const levelID = level->m_levelID.value();
    std::string const levelName = level->m_levelName.c_str();
    std::string const creatorName = level->m_creatorName.c_str();

    // Server and official levels have a stable numeric ID. Always prefer it
    // over level data, which may be reserialized between the info and play scenes.
    if (levelID != 0) {
        return { fmt::format("rod-ball-level-id-{}", levelID) };
    }

    // Local levels do not have a server ID. Prefer the stable local identity
    // (name + creator) so re-serializing level data cannot change the selection.
    // Keep the previous content-hash key second for migration from v1.6.0.
    std::string const levelData = level->m_levelString.c_str();
    std::string const localIdentity = levelName + "|" + creatorName;
    std::string const contentIdentity = localIdentity + "|" + levelData;
    return {
        fmt::format("rod-ball-local-{:016x}", stableHash(localIdentity)),
        fmt::format("rod-ball-content-{:016x}", stableHash(contentIdentity))
    };
}

int selectedRodBall(std::vector<std::string> const& keys) {
    for (auto const& key : keys) {
        int const value = Mod::get()->getSavedValue<int>(key, -1);
        if (value >= 0 && value <= 3) {
            return value;
        }
    }
    return DEFAULT_ROD_BALL;
}

// The button always shows a Rod preview. Normal is the gameplay choice,
// but its button preview intentionally falls back to Rod 3.
int rodBallForButton(std::vector<std::string> const& keys) {
    for (auto const& key : keys) {
        int const value = Mod::get()->getSavedValue<int>(key, -1);
        if (value >= 1 && value <= 3) return value;
        if (value == 0) return 3;
    }
    return 3;
}

CCNode* createRodSelectorButtonSprite(int selection) {
    auto const backgroundPath = (Mod::get()->getResourcesDir() / "GJ_plainBtn_001.png").string();
    auto* background = CCSprite::create(backgroundPath.c_str());
    if (!background) {
        log::warn("Unable to load Rod selector button background: {}", backgroundPath);
        return nullptr;
    }

    // Enlarge the actual sprite and its container instead of scaling only the
    // menu item. CCMenuItemSpriteExtra then receives the full enlarged content
    // size, so both the visible button and its touch target are 3.5x larger.
    constexpr float backgroundScale = 1.015f; // 0.29f * 3.5f
    background->setScale(backgroundScale);
    auto const size = CCSize{
        background->getContentSize().width * backgroundScale,
        background->getContentSize().height * backgroundScale
    };

    auto* container = CCNode::create();
    if (!container) return nullptr;
    container->setContentSize(size);
    background->setPosition({ size.width / 2.f, size.height / 2.f });
    container->addChild(background, 0);

    // Draw the native Rod assembly on top of the supplied green button:
    // the rod stem (rod_0X) below the ball (rod_ball_0X).
    if (selection >= 1 && selection <= 3) {
        auto const stemFrame = fmt::format("rod_0{}_001.png", selection);
        auto* stem = CCSprite::createWithSpriteFrameName(stemFrame.c_str());
        if (stem) {
            stem->setScale(0.42f);
            stem->setAnchorPoint({ 0.5f, 1.f });
            stem->setPosition({ size.width / 2.f, size.height * 0.39f });
            container->addChild(stem, 1);
        } else {
            log::warn("Unable to load native Rod stem frame: {}", stemFrame);
        }

        auto* ball = CCSprite::createWithSpriteFrameName(ROD_BALL_FRAME_NAMES[selection - 1]);
        if (ball) {
            ball->setScale(0.98f);
            ball->setPosition({ size.width / 2.f, size.height * 0.69f });
            container->addChild(ball, 2);
        } else {
            log::warn("Unable to load native Rod Ball frame: {}", ROD_BALL_FRAME_NAMES[selection - 1]);
        }
    }

    return container;
}

CCSprite* createRodBallPreview(int selection) {
    // Normal intentionally has no preview image.
    if (selection < 1 || selection > 3) return nullptr;

    // Read the packaged raw PNGs for the popup preview. The in-level Rod itself
    // uses Geometry Dash's native GJ_GameSheet frames (see the CCSprite hook).
    auto const path = Mod::get()->getResourcesDir() / ROD_BALL_FILES[selection - 1];
    auto const fullPath = path.string();
    auto* sprite = CCSprite::create(fullPath.c_str());
    if (!sprite) {
        log::warn("Unable to load Rod Ball preview image: {}", fullPath);
    }
    return sprite;
}

void addRodSelectorButton(
    CCNode* layer, CCObject* target, SEL_MenuHandler callback, GJGameLevel* level
) {
    if (!layer || layer->getChildByID("rod-ball-selector-menu"_spr)) return;

    auto* buttonSprite = createRodSelectorButtonSprite(
        rodBallForButton(levelStorageKeys(level))
    );
    if (!buttonSprite) return;

    auto* button = CCMenuItemSpriteExtra::create(buttonSprite, target, callback);
    if (!button) return;
    button->setID("rod-ball-selector-open-button"_spr);

    // Give the menu an explicit, centered hit area matching the enlarged button.
    auto* menu = CCMenu::createWithItem(button);
    if (!menu) return;
    menu->setID("rod-ball-selector-menu"_spr);
    auto const buttonSize = button->getScaledContentSize();
    menu->setContentSize(buttonSize);
    menu->setAnchorPoint({ 0.5f, 0.5f });
    button->setPosition({ buttonSize.width / 2.f, buttonSize.height / 2.f });

    auto const winSize = CCDirector::sharedDirector()->getWinSize();
    CCPoint selectorPosition = {
        winSize.width * 0.70f,
        winSize.height * 0.82f
    };

    // LevelInfoLayer's Node IDs expose the Cube/profile control as:
    // layer -> "garage-menu" -> "garage-button".
    // Use the actual button's world position and dimensions instead of a
    // guessed screen percentage, which caused the enlarged selector to
    // overlap the Cube and extend beyond the top edge.
    auto* cubeMenu = layer->getChildByID("garage-menu");
    auto* cubeButton = cubeMenu ? cubeMenu->getChildByID("garage-button") : nullptr;
    if (cubeButton && cubeButton->getParent()) {
        auto const cubeWorldPosition = cubeButton->getParent()->convertToWorldSpace(
            cubeButton->getPosition()
        );
        auto const cubePosition = layer->convertToNodeSpace(cubeWorldPosition);
        auto const cubeSize = cubeButton->getScaledContentSize();
        auto const selectorSize = button->getScaledContentSize();
        constexpr float gap = 12.f;
        constexpr float edgeMargin = 12.f;

        selectorPosition.x = cubePosition.x - cubeSize.width / 2.f
            - selectorSize.width / 2.f - gap;

        // Keep the larger selector completely visible. The Cube is close to
        // the top edge, so the selector center must be lowered when necessary.
        float const halfHeight = selectorSize.height / 2.f;
        selectorPosition.y = std::clamp(
            cubePosition.y,
            halfHeight + edgeMargin,
            winSize.height - halfHeight - edgeMargin
        );

        log::info(
            "Rod Ball Selector anchored left of Cube: cube=({:.1f}, {:.1f}), selector=({:.1f}, {:.1f}), size=({:.1f}, {:.1f})",
            cubePosition.x, cubePosition.y,
            selectorPosition.x, selectorPosition.y,
            selectorSize.width, selectorSize.height
        );
    } else {
        // LevelPage does not expose the same garage-menu node. Use a safe
        // in-bounds fallback there, and report the missing anchor clearly.
        log::warn("Rod Ball Selector could not find LevelInfoLayer Node ID 'garage-menu/garage-button'; using safe fallback position");
        float const halfHeight = button->getScaledContentSize().height / 2.f;
        selectorPosition.y = std::clamp(
            selectorPosition.y,
            halfHeight + 12.f,
            winSize.height - halfHeight - 12.f
        );
    }

    menu->setPosition(selectorPosition);
    layer->addChild(menu, 10000);
}

class RodBallPopup : public geode::Popup {
protected:
    std::vector<std::string> m_storageKeys;
    CCNode* m_levelLayer = nullptr;

    bool init(std::vector<std::string> const& storageKeys, CCNode* levelLayer) {
        if (!geode::Popup::init(350.f, 190.f)) return false;
        m_storageKeys = storageKeys;
        m_levelLayer = levelLayer;

        setTitle("Choose Rod Ball");
        if (m_title) {
            m_title->setLimitLabelWidth(m_mainLayer->getContentSize().width - 55.f, 0.65f, 0.1f);
        }

        auto* menu = CCMenu::create();
        if (!menu) return true;
        menu->setPosition({
            m_mainLayer->getContentSize().width / 2.f,
            m_mainLayer->getContentSize().height / 2.f - 8.f
        });

        std::array<char const*, 4> const labels = { "Normal", "Rod 1", "Rod 2", "Rod 3" };
        std::array<float, 4> const positions = { -124.f, -42.f, 42.f, 124.f };
        int const currentSelection = selectedRodBall(m_storageKeys);

        for (int i = 0; i < 4; ++i) {
            auto* card = CCNode::create();
            if (!card) continue;
            card->setContentSize({ 74.f, 82.f });

            auto* preview = createRodBallPreview(i);
            if (preview) {
                preview->setScale(i == 0 ? 0.82f : 0.72f);
                preview->setPosition({ 37.f, 52.f });
                card->addChild(preview);
            }

            auto* label = CCLabelBMFont::create(labels[i], "goldFont.fnt");
            if (label) {
                label->setScale(0.43f);
                label->setPosition({ 37.f, 10.f });
                if (currentSelection == i) label->setColor(ccc3(90, 255, 120));
                card->addChild(label);
            }

            auto* button = CCMenuItemSpriteExtra::create(
                card, this, menu_selector(RodBallPopup::onChoose)
            );
            if (!button) continue;
            button->setTag(i);
            button->setPosition({ positions[i], 0.f });
            menu->addChild(button);
        }

        m_mainLayer->addChild(menu);
        return true;
    }

public:
    static RodBallPopup* create(std::vector<std::string> const& storageKeys, CCNode* levelLayer) {
        auto* ret = new RodBallPopup();
        if (ret->init(storageKeys, levelLayer)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    void onChoose(CCObject* sender) {
        auto* button = static_cast<CCMenuItemSpriteExtra*>(sender);
        int const selection = button->getTag();
        if (selection < 0 || selection > 3) return;

        for (auto const& key : m_storageKeys) {
            Mod::get()->setSavedValue<int>(key, selection);
        }

        // Refresh the level-page button immediately after saving the choice.
        // Normal gameplay uses the vanilla Rod, but the button preview falls back to Rod 3.
        if (m_levelLayer) {
            auto* menu = m_levelLayer->getChildByID("rod-ball-selector-menu"_spr);
            auto* buttonNode = menu ? menu->getChildByID("rod-ball-selector-open-button"_spr) : nullptr;
            auto* button = typeinfo_cast<CCMenuItemSpriteExtra*>(buttonNode);
            auto* preview = createRodSelectorButtonSprite(selection == 0 ? 3 : selection);
            if (button && preview) {
                button->setNormalImage(preview);
                button->updateSprite();
            }
        }

        Notification::create(
            selection == 0 ? "Normal Ball selected" : fmt::format("Rod Ball {} selected", selection),
            NotificationIcon::Success
        )->show();
        onClose(nullptr);
    }
};


size_t resetAllLevelSelectionsToNormal() {
    auto* mod = Mod::get();
    auto& saveContainer = mod->getSaveContainer();
    std::vector<std::string> levelKeys;

    if (saveContainer.isObject()) {
        for (auto const& [key, value] : saveContainer) {
            bool const isLevelKey =
                key == "rod-ball-level-unknown" ||
                key.starts_with("rod-ball-level-id-") ||
                key.starts_with("rod-ball-local-") ||
                key.starts_with("rod-ball-content-");
            if (isLevelKey) levelKeys.push_back(key);
        }
    }

    // Store an explicit Normal selection (0) for every known level. Do not
    // touch unrelated settings or saved values belonging to other features.
    for (auto const& key : levelKeys) {
        mod->setSavedValue<int>(key, DEFAULT_ROD_BALL);
    }

    auto saveResult = mod->saveData();
    if (!saveResult) {
        log::warn("Failed to immediately save reset Rod selections: {}", saveResult.unwrapErr());
    }
    return levelKeys.size();
}

} // namespace

// Geode's built-in button setting raises this event when the Settings button is pressed.
// Using the native button type avoids an unresolved custom-setting entry in the mod page.
$on_mod(Loaded) {
    ButtonSettingPressedEventV3(Mod::get(), "reset-all-levels").listen([](auto buttonKey) {
        if (buttonKey != "reset-to-normal") return;

        createQuickPopup(
            "Reset all Rod selections?",
            "This will set every saved level's Rod selection to <cy>Normal</c>. "
            "This action cannot be undone.",
            "Cancel",
            "Reset",
            [](auto, bool confirmed) {
                if (!confirmed) return;

                size_t const resetCount = resetAllLevelSelectionsToNormal();
                Notification::create(
                    fmt::format("Reset {} level selection(s) to Normal", resetCount),
                    NotificationIcon::Success
                )->show();
            }
        );
    }).leak();
}

// Online and locally saved user levels use LevelInfoLayer.
class $modify(RodBallLevelInfo, LevelInfoLayer) {
    void setupLevelInfo() {
        LevelInfoLayer::setupLevelInfo();
        geode::NodeIDs::provideFor(this);
        addRodSelectorButton(
            this, this, menu_selector(RodBallLevelInfo::onOpenRodBallPopup), m_level
        );
    }

    void onOpenRodBallPopup(CCObject*) {
        auto* popup = RodBallPopup::create(levelStorageKeys(m_level), this);
        if (popup) popup->show();
    }
};

// Official levels are displayed through LevelPage rather than LevelInfoLayer.
class $modify(RodBallLevelPage, LevelPage) {
    bool init(GJGameLevel* level) {
        if (!LevelPage::init(level)) return false;
        geode::NodeIDs::provideFor(this);
        addRodSelectorButton(
            this, this, menu_selector(RodBallLevelPage::onOpenRodBallPopup), m_level
        );
        return true;
    }

    void onOpenRodBallPopup(CCObject*) {
        auto* popup = RodBallPopup::create(levelStorageKeys(m_level), this);
        if (popup) popup->show();
    }
};

class $modify(RodBallPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool dontCreateObjects) {
        // Capture the selected variant before the original PlayLayer init starts
        // creating level objects and their Rod Ball sprites.
        g_activeRodBall = selectedRodBall(levelStorageKeys(level));
        return PlayLayer::init(level, dontCreateObjects);
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        geode::NodeIDs::provideFor(this);
    }

    void onExit() {
        PlayLayer::onExit();
        g_activeRodBall = DEFAULT_ROD_BALL;
    }
};

class $modify(RodBallSprite, CCSprite) {
    bool initWithSpriteFrameName(const char* frameName) {
        // The three Rod Ball images are native GJ_GameSheet frames. Intercept
        // the actual sprite initialization so all Rod objects use the same
        // saved selection, including when a retry recreates the Rod sprite.
        if (frameName && g_activeRodBall >= 1 && g_activeRodBall <= 3) {
            bool const isRodBallFrame =
                std::strcmp(frameName, "rod_ball_01_001.png") == 0 ||
                std::strcmp(frameName, "rod_ball_02_001.png") == 0 ||
                std::strcmp(frameName, "rod_ball_03_001.png") == 0;

            if (isRodBallFrame) {
                frameName = ROD_BALL_FRAME_NAMES[g_activeRodBall - 1];
            }
        }

        return CCSprite::initWithSpriteFrameName(frameName);
    }
};