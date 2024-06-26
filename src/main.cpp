#include "main.hpp"
#include "library_utils.hpp"

#include "SettingsViewController.hpp"
#include "ModListViewController.hpp"
using namespace ModList;

#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML.hpp"
using namespace BSML;

#include "beatsaber-hook/shared/utils/utils-functions.h"

#include "GlobalNamespace/MainMenuViewController.hpp"
using namespace GlobalNamespace;

#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/TextAnchor.hpp"
using namespace UnityEngine::UI;

#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
using namespace TMPro;

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

// Displays a modal view if mods fail to load showing why
MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    PaperLogger.info("MainMenuViewController_DidActivate");
    if(!firstActivation) {
        PaperLogger.info("Not first activation, not displaying modal");
        return;
    }

    if(!getConfig().config["showFailedModsOnGameStart"].GetBool()) {
        PaperLogger.info("Showing failed mods on game start is disabled! Returning");
        return;
    }

    PaperLogger.info("Checking for failed mods . . .");
    LibraryLoadInfo& modsLoadInfo = GetModsLoadInfo();
    LibraryLoadInfo& earlyModsLoadInfo = GetEarlyModsLoadInfo();

    std::unordered_map<std::string, std::string> failedMods;
    for(std::pair<std::string, std::optional<std::string>> modPair : modsLoadInfo) {
        if(modPair.second.has_value()) {
            failedMods[modPair.first] = *modPair.second;
        }
    }
    std::unordered_map<std::string, std::string> failedEarlyMods;
    for(std::pair<std::string, std::optional<std::string>> modPair : earlyModsLoadInfo) {
        if(modPair.second.has_value()) {
            failedEarlyMods[modPair.first] = *modPair.second;
        }
    }

    PaperLogger.info("%lu mods failed to load", failedMods.size());

    PaperLogger.info("%lu early mods failed to load", failedEarlyMods.size());

    if(failedMods.empty() && failedEarlyMods.empty()) {
        PaperLogger.info("All mods loaded successfully, not showing fail dialog");
        return;
    }

    PaperLogger.info("Constructing fail dialog . . .");

    BSML::ModalView* modalView = Lite::CreateModal(self->get_transform(), {}, UnityEngine::Vector2(70.0f, 70.0f), nullptr);
    modalView->onHide = [modalView]() {
        PaperLogger.info("Fail dialog closed, destroying modal view!");
        UnityEngine::GameObject::Destroy(modalView->get_gameObject());
    };
    UnityEngine::Transform* modalTransform = modalView->get_transform();
    VerticalLayoutGroup* layout = Lite::CreateVerticalLayoutGroup(modalTransform);
    UnityEngine::RectTransform* layoutTransform = layout->get_rectTransform();

    layout->set_padding(UnityEngine::RectOffset::New_ctor(2, 2, 4, 4));
    layout->set_childAlignment(UnityEngine::TextAnchor::UpperLeft);
    layout->set_childControlHeight(true);
    layout->set_childForceExpandHeight(false);
    layout->set_childControlWidth(false);
    layout->set_childForceExpandWidth(true);

    std::string failedModsText, failedEarlyModsText;
    // Make sure to adjust for the plural!
    if(failedMods.size() > 1) {
        failedModsText = fmt::format("{}u mods failed to load!", failedMods.size());
    }   else    {
        failedModsText = fmt::format("{}u mod failed to load!", failedMods.size());
    }
    if(failedEarlyMods.size() > 1) {
        failedEarlyModsText = fmt::format("{}u early mods failed to load!", failedEarlyMods.size());
    }   else    {
        failedEarlyModsText = fmt::format("{}u early mod failed to load!", failedEarlyMods.size());
    }

    if(failedMods.size() > 0) {
        TextMeshProUGUI* modsTitleText = Lite::CreateText(layoutTransform, failedModsText);
        modsTitleText->set_fontSize(5.0f);
        modsTitleText->set_alignment(TextAlignmentOptions::TopLeft);

        Lite::CreateText(layoutTransform, "_______________________________");

        // Add the failed mods to the GUI
        for(std::pair<std::string, std::string> failedMod : failedMods) {
            TextMeshProUGUI* modText = Lite::CreateText(layoutTransform, fmt::format("<color=red>{}</color> didn't load because: \n{}", failedMod.first.c_str(), failedMod.second.c_str()));
            modText->set_overflowMode(TextOverflowModes::Ellipsis);
            modText->set_fontSize(3.5f);

            Lite::AddHoverHint(modText->get_gameObject(), failedMod.second); // Show the full fail reason in a hover hint, since there most likely won't be enough space in the modal view
        }
    }

    if(failedEarlyMods.size() > 0) {
        TextMeshProUGUI* earlyModsTitleText = Lite::CreateText(layoutTransform, failedEarlyModsText);
        earlyModsTitleText->set_fontSize(5.0f);
        earlyModsTitleText->set_alignment(TextAlignmentOptions::TopLeft);

        Lite::CreateText(layoutTransform, "_______________________________");

        // Add the failed mods to the GUI
        for(std::pair<std::string, std::string> failedMod : failedEarlyMods) {
            TextMeshProUGUI* modText = Lite::CreateText(layoutTransform, fmt::format("<color=red>{}</color> didn't load because: \n{}", failedMod.first.c_str(), failedMod.second.c_str()));
            modText->set_overflowMode(TextOverflowModes::Ellipsis);
            modText->set_fontSize(3.5f);

            Lite::AddHoverHint(modText->get_gameObject(), failedMod.second); // Show the full fail reason in a hover hint, since there most likely won't be enough space in the modal view
        }
    }

    PaperLogger.info("Showing fail dialog . . .");
    modalView->Show();
}

void ConstructConfig() {
    ConfigDocument& config = getConfig().config;
    auto& alloc = config.GetAllocator();
    if(!config.HasMember("showFailedModsOnGameStart")) {
        config.AddMember("showFailedModsOnGameStart", true, alloc);
        getConfig().Write();
    }
}

// Called at the early stages of game loading
MODLIST_EXPORT void setup(CModInfo* info) {
    *info = modInfo.to_c();
	
    getConfig().Load(); // Load the config file
    ConstructConfig(); // Add properties to the config if missing

    PaperLogger.info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
MODLIST_EXPORT void late_load() {
    il2cpp_functions::Init();

    // Register our mod settings menu
    BSML::Init();
    custom_types::Register::AutoRegister();
    BSML::Register::RegisterMainMenu<ModListViewController*>("Loaded Mods", "Loaded Mods", "View Loaded Mods");
    BSML::Register::RegisterSettingsMenu<SettingsViewController*>("Mod List Settings");

    PaperLogger.info("Installing hooks...");
    INSTALL_HOOK(PaperLogger, MainMenuViewController_DidActivate);
    PaperLogger.info("Installed all hooks!");
}