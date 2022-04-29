/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/tl/string.h>

#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/updater.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/chat.h>
#include <game/client/components/menu_background.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/localization.h>

#include "base/system.h"
#include "binds.h"
#include "camera.h"
#include "countryflags.h"
#include "menus.h"
#include "skins.h"

#include <memory>
#include <utility>
#include <vector>

#include <array>
#include <numeric>

CMenusKeyBinder CMenus::m_Binder;

CMenusKeyBinder::CMenusKeyBinder()
{
	m_TakeKey = false;
	m_GotKey = false;
	m_ModifierCombination = 0;
}

bool CMenusKeyBinder::OnInput(IInput::CEvent Event)
{
	if(m_TakeKey)
	{
		int TriggeringEvent = (Event.m_Key == KEY_MOUSE_1) ? IInput::FLAG_PRESS : IInput::FLAG_RELEASE;
		if(Event.m_Flags & TriggeringEvent)
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;

			m_ModifierCombination = CBinds::GetModifierMask(Input());
			if(m_ModifierCombination == CBinds::GetModifierMaskOfKey(Event.m_Key))
			{
				m_ModifierCombination = 0;
			}
		}
		return true;
	}

	return false;
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
#if defined(CONF_FAMILY_WINDOWS)
	bool CheckSettings = false;
	static int s_ClShowConsole = g_Config.m_ClShowConsole;
#endif

	char aBuf[128 + IO_MAX_PATH_LENGTH];
	CUIRect Label, Button, Left, Right, Game, Client;
	MainView.HSplitTop(150.0f, &Game, &Client);

	// game
	{
		// headline
		Game.HSplitTop(20.0f, &Label, &Game);
		UI()->DoLabelScaled(&Label, Localize("Game"), 20.0f, TEXTALIGN_LEFT);
		Game.Margin(5.0f, &Game);
		Game.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// dynamic camera
		Left.HSplitTop(20.0f, &Button, &Left);
		bool IsDyncam = g_Config.m_ClDyncam || g_Config.m_ClMouseFollowfactor > 0;
		if(DoButton_CheckBox(&g_Config.m_ClDyncam, Localize("Dynamic Camera"), IsDyncam, &Button))
		{
			if(IsDyncam)
			{
				g_Config.m_ClDyncam = 0;
				g_Config.m_ClMouseFollowfactor = 0;
			}
			else
			{
				g_Config.m_ClDyncam = 1;
			}
		}

		// smooth dynamic camera
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(g_Config.m_ClDyncam)
		{
			if(DoButton_CheckBox(&g_Config.m_ClDyncamSmoothness, Localize("Smooth Dynamic Camera"), g_Config.m_ClDyncamSmoothness, &Button))
			{
				if(g_Config.m_ClDyncamSmoothness)
				{
					g_Config.m_ClDyncamSmoothness = 0;
				}
				else
				{
					g_Config.m_ClDyncamSmoothness = 50;
					g_Config.m_ClDyncamStabilizing = 50;
				}
			}
		}

		// weapon pickup
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClAutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
			g_Config.m_ClAutoswitchWeapons ^= 1;

		// weapon out of ammo autoswitch
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClAutoswitchWeaponsOutOfAmmo, Localize("Switch weapon when out of ammo"), g_Config.m_ClAutoswitchWeaponsOutOfAmmo, &Button))
			g_Config.m_ClAutoswitchWeaponsOutOfAmmo ^= 1;

		// weapon reset on death
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClResetWantedWeaponOnDeath, Localize("Reset wanted weapon on death"), g_Config.m_ClResetWantedWeaponOnDeath, &Button))
			g_Config.m_ClResetWantedWeaponOnDeath ^= 1;

		// chat messages
		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClShowChatFriends, Localize("Show only chat messages from friends"), g_Config.m_ClShowChatFriends, &Button))
			g_Config.m_ClShowChatFriends ^= 1;

		// name plates
		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClNameplates, Localize("Show name plates"), g_Config.m_ClNameplates, &Button))
			g_Config.m_ClNameplates ^= 1;

		if(g_Config.m_ClNameplates)
		{
			Right.HSplitTop(2.5f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Name plates size"), g_Config.m_ClNameplatesSize);
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
			g_Config.m_ClNameplatesSize = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClNameplatesSize, &Button, g_Config.m_ClNameplatesSize / 100.0f) * 100.0f + 0.1f);

			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClNameplatesTeamcolors, Localize("Use team colors for name plates"), g_Config.m_ClNameplatesTeamcolors, &Button))
				g_Config.m_ClNameplatesTeamcolors ^= 1;

			Right.HSplitTop(5.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClNameplatesClan, Localize("Show clan above name plates"), g_Config.m_ClNameplatesClan, &Button))
				g_Config.m_ClNameplatesClan ^= 1;

			if(g_Config.m_ClNameplatesClan)
			{
				Right.HSplitTop(2.5f, 0, &Right);
				Right.HSplitTop(20.0f, &Label, &Right);
				Right.HSplitTop(20.0f, &Button, &Right);
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Clan plates size"), g_Config.m_ClNameplatesClanSize);
				UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
				g_Config.m_ClNameplatesClanSize = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClNameplatesClanSize, &Button, g_Config.m_ClNameplatesClanSize / 100.0f) * 100.0f + 0.1f);
			}
		}
	}

	// client
	{
		// headline
		Client.HSplitTop(20.0f, &Label, &Client);
		UI()->DoLabelScaled(&Label, Localize("Client"), 20.0f, TEXTALIGN_LEFT);
		Client.Margin(5.0f, &Client);
		Client.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// skip main menu
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClSkipStartMenu, Localize("Skip the main menu"), g_Config.m_ClSkipStartMenu, &Button))
			g_Config.m_ClSkipStartMenu ^= 1;

		float SliderGroupMargin = 10.0f;

		// auto demo settings
		{
			Right.HSplitTop(40.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button))
				g_Config.m_ClAutoDemoRecord ^= 1;

			Right.HSplitTop(20.0f, &Label, &Right);
			if(g_Config.m_ClAutoDemoMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max demos"), g_Config.m_ClAutoDemoMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max demos"), "∞");
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
			Right.HSplitTop(20.0f, &Button, &Right);
			g_Config.m_ClAutoDemoMax = static_cast<int>(UIEx()->DoScrollbarH(&g_Config.m_ClAutoDemoMax, &Button, g_Config.m_ClAutoDemoMax / 1000.0f) * 1000.0f + 0.1f);

			Right.HSplitTop(SliderGroupMargin, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button))
				g_Config.m_ClAutoScreenshot ^= 1;

			Right.HSplitTop(20.0f, &Label, &Right);
			if(g_Config.m_ClAutoScreenshotMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_ClAutoScreenshotMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), "∞");
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
			Right.HSplitTop(20.0f, &Button, &Right);
			g_Config.m_ClAutoScreenshotMax = static_cast<int>(UIEx()->DoScrollbarH(&g_Config.m_ClAutoScreenshotMax, &Button, g_Config.m_ClAutoScreenshotMax / 1000.0f) * 1000.0f + 0.1f);
		}

		Left.HSplitTop(10.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Label, &Left);
		if(g_Config.m_ClRefreshRate)
			str_format(aBuf, sizeof(aBuf), "%s: %i Hz", Localize("Refresh Rate"), g_Config.m_ClRefreshRate);
		else
			str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Refresh Rate"), "∞");
		UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
		Left.HSplitTop(20.0f, &Button, &Left);
		g_Config.m_ClRefreshRate = static_cast<int>(UIEx()->DoScrollbarH(&g_Config.m_ClRefreshRate, &Button, g_Config.m_ClRefreshRate / 10000.0f) * 10000.0f + 0.1f);

#if defined(CONF_FAMILY_WINDOWS)
		Left.HSplitTop(10.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		if(DoButton_CheckBox(&g_Config.m_ClShowConsole, Localize("Show console window"), g_Config.m_ClShowConsole, &Button))
		{
			g_Config.m_ClShowConsole ^= 1;
			CheckSettings = true;
		}

		if(CheckSettings)
			m_NeedRestartGeneral = s_ClShowConsole != g_Config.m_ClShowConsole;
#endif

		Left.HSplitTop(15.0f, 0, &Left);
		CUIRect SettingsButton;
		Left.HSplitBottom(25.0f, &Left, &SettingsButton);

		SettingsButton.HSplitTop(5.0f, 0, &SettingsButton);
		static int s_SettingsButtonID = 0;
		if(DoButton_Menu(&s_SettingsButtonID, Localize("Settings file"), 0, &SettingsButton))
		{
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, CONFIG_FILE, aBuf, sizeof(aBuf));
			if(!open_file(aBuf))
			{
				dbg_msg("menus", "couldn't open file");
			}
		}

		Left.HSplitTop(15.0f, 0, &Left);
		CUIRect ConfigButton;
		Left.HSplitBottom(25.0f, &Left, &ConfigButton);

		ConfigButton.HSplitTop(5.0f, 0, &ConfigButton);
		static int s_ConfigButtonID = 0;
		if(DoButton_Menu(&s_ConfigButtonID, Localize("Config directory"), 0, &ConfigButton))
		{
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, "", aBuf, sizeof(aBuf));
			if(!open_file(aBuf))
			{
				dbg_msg("menus", "couldn't open file");
			}
		}

		Left.HSplitTop(15.0f, 0, &Left);
		CUIRect DirectoryButton;
		Left.HSplitBottom(25.0f, &Left, &DirectoryButton);
		RenderThemeSelection(Left);

		DirectoryButton.HSplitTop(5.0f, 0, &DirectoryButton);
		static int s_ThemesButtonID = 0;
		if(DoButton_Menu(&s_ThemesButtonID, Localize("Themes directory"), 0, &DirectoryButton))
		{
			Storage()->GetCompletePath(IStorage::TYPE_SAVE, "themes", aBuf, sizeof(aBuf));
			Storage()->CreateFolder("themes", IStorage::TYPE_SAVE);
			if(!open_file(aBuf))
			{
				dbg_msg("menus", "couldn't open file");
			}
		}

		// auto statboard screenshot
		{
			Right.HSplitTop(SliderGroupMargin, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoStatboardScreenshot,
				   Localize("Automatically take statboard screenshot"),
				   g_Config.m_ClAutoStatboardScreenshot, &Button))
			{
				g_Config.m_ClAutoStatboardScreenshot ^= 1;
			}

			Right.HSplitTop(20.0f, &Label, &Right);
			if(g_Config.m_ClAutoStatboardScreenshotMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_ClAutoStatboardScreenshotMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), "∞");
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
			Right.HSplitTop(20.0f, &Button, &Right);
			g_Config.m_ClAutoStatboardScreenshotMax =
				static_cast<int>(UIEx()->DoScrollbarH(&g_Config.m_ClAutoStatboardScreenshotMax,
							 &Button,
							 g_Config.m_ClAutoStatboardScreenshotMax / 1000.0f) *
							 1000.0f +
						 0.1f);
		}

		// auto statboard csv
		{
			Right.HSplitTop(SliderGroupMargin, 0, &Right); //
			Right.HSplitTop(20.0f, &Button, &Right);
			if(DoButton_CheckBox(&g_Config.m_ClAutoCSV,
				   Localize("Automatically create statboard csv"),
				   g_Config.m_ClAutoCSV, &Button))
			{
				g_Config.m_ClAutoCSV ^= 1;
			}

			Right.HSplitTop(20.0f, &Label, &Right);
			if(g_Config.m_ClAutoCSVMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max CSVs"), g_Config.m_ClAutoCSVMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max CSVs"), "∞");
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
			Right.HSplitTop(20.0f, &Button, &Right);
			g_Config.m_ClAutoCSVMax =
				static_cast<int>(UIEx()->DoScrollbarH(&g_Config.m_ClAutoCSVMax,
							 &Button,
							 g_Config.m_ClAutoCSVMax / 1000.0f) *
							 1000.0f +
						 0.1f);
		}
	}
}

void CMenus::SetNeedSendInfo()
{
	if(m_Dummy)
		m_NeedSendDummyinfo = true;
	else
		m_NeedSendinfo = true;
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	CUIRect Button, Label, Dummy;
	MainView.HSplitTop(10.0f, 0, &MainView);

	char *pName = g_Config.m_PlayerName;
	const char *pNameFallback = Client()->PlayerName();
	char *pClan = g_Config.m_PlayerClan;
	int *pCountry = &g_Config.m_PlayerCountry;

	if(m_Dummy)
	{
		pName = g_Config.m_ClDummyName;
		pNameFallback = Client()->DummyName();
		pClan = g_Config.m_ClDummyClan;
		pCountry = &g_Config.m_ClDummyCountry;
	}

	// player name
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);
	static float s_OffsetName = 0.0f;
	SUIExEditBoxProperties EditProps;
	EditProps.m_pEmptyText = pNameFallback;
	if(UIEx()->DoEditBox(pName, &Button, pName, sizeof(g_Config.m_PlayerName), 14.0f, &s_OffsetName, false, CUI::CORNER_ALL, EditProps))
	{
		SetNeedSendInfo();
	}

	// player clan
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(200.0f, &Button, &Dummy);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);
	static float s_OffsetClan = 0.0f;
	if(UIEx()->DoEditBox(pClan, &Button, pClan, sizeof(g_Config.m_PlayerClan), 14.0f, &s_OffsetClan))
	{
		SetNeedSendInfo();
	}

	if(DoButton_CheckBox(&m_Dummy, Localize("Dummy settings"), m_Dummy, &Dummy))
	{
		m_Dummy ^= 1;
	}

	static bool s_ListBoxUsed = false;
	if(UI()->ActiveItem() == pClan || UI()->ActiveItem() == pName)
		s_ListBoxUsed = false;

	// country flag selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	UiDoListboxStart(&s_ScrollValue, &MainView, 50.0f, Localize("Country / Region"), "", m_pClient->m_CountryFlags.Num(), 6, OldSelected, s_ScrollValue);

	for(int i = 0; i < m_pClient->m_CountryFlags.Num(); ++i)
	{
		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_CountryFlags.GetByIndex(i);
		if(pEntry->m_CountryCode == *pCountry)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&pEntry->m_CountryCode, OldSelected == i, s_ListBoxUsed);
		if(Item.m_Visible)
		{
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);
			float OldWidth = Item.m_Rect.w;
			Item.m_Rect.w = Item.m_Rect.h * 2;
			Item.m_Rect.x += (OldWidth - Item.m_Rect.w) / 2.0f;
			ColorRGBA Color(1.0f, 1.0f, 1.0f, 1.0f);
			m_pClient->m_CountryFlags.Render(pEntry->m_CountryCode, &Color, Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
			if(pEntry->m_Texture.IsValid())
				UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, TEXTALIGN_CENTER);
		}
	}

	bool Clicked = false;
	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0, &Clicked);
	if(Clicked)
		s_ListBoxUsed = true;

	if(OldSelected != NewSelected)
	{
		*pCountry = m_pClient->m_CountryFlags.GetByIndex(NewSelected)->m_CountryCode;
		SetNeedSendInfo();
	}
}

struct CUISkin
{
	const CSkin *m_pSkin;

	CUISkin() :
		m_pSkin(nullptr) {}
	CUISkin(const CSkin *pSkin) :
		m_pSkin(pSkin) {}

	bool operator<(const CUISkin &Other) const { return str_comp_nocase(m_pSkin->m_aName, Other.m_pSkin->m_aName) < 0; }

	bool operator<(const char *pOther) const { return str_comp_nocase(m_pSkin->m_aName, pOther) < 0; }
	bool operator==(const char *pOther) const { return !str_comp_nocase(m_pSkin->m_aName, pOther); }
};

void CMenus::RenderSettingsTee(CUIRect MainView)
{
	CUIRect Button, Label, Dummy, DummyLabel, SkinList, QuickSearch, QuickSearchClearButton, SkinDB, SkinPrefix, SkinPrefixLabel, DirectoryButton, RefreshButton, Eyes, EyesLabel, EyesTee, EyesRight;

	static bool s_InitSkinlist = true;
	MainView.HSplitTop(10.0f, 0, &MainView);
	Eyes = MainView;

	char *pSkinName = g_Config.m_ClPlayerSkin;
	int *UseCustomColor = &g_Config.m_ClPlayerUseCustomColor;
	unsigned *ColorBody = &g_Config.m_ClPlayerColorBody;
	unsigned *ColorFeet = &g_Config.m_ClPlayerColorFeet;

	if(m_Dummy)
	{
		pSkinName = g_Config.m_ClDummySkin;
		UseCustomColor = &g_Config.m_ClDummyUseCustomColor;
		ColorBody = &g_Config.m_ClDummyColorBody;
		ColorFeet = &g_Config.m_ClDummyColorFeet;
	}

	// skin info
	CTeeRenderInfo OwnSkinInfo;
	const CSkin *pSkin = m_pClient->m_Skins.Get(m_pClient->m_Skins.Find(pSkinName));
	OwnSkinInfo.m_OriginalRenderSkin = pSkin->m_OriginalSkin;
	OwnSkinInfo.m_ColorableRenderSkin = pSkin->m_ColorableSkin;
	OwnSkinInfo.m_SkinMetrics = pSkin->m_Metrics;
	OwnSkinInfo.m_CustomColoredSkin = *UseCustomColor;
	if(*UseCustomColor)
	{
		OwnSkinInfo.m_ColorBody = color_cast<ColorRGBA>(ColorHSLA(*ColorBody).UnclampLighting());
		OwnSkinInfo.m_ColorFeet = color_cast<ColorRGBA>(ColorHSLA(*ColorFeet).UnclampLighting());
	}
	else
	{
		OwnSkinInfo.m_ColorBody = ColorRGBA(1.0f, 1.0f, 1.0f);
		OwnSkinInfo.m_ColorFeet = ColorRGBA(1.0f, 1.0f, 1.0f);
	}
	OwnSkinInfo.m_Size = 50.0f * UI()->Scale();

	MainView.HSplitTop(20.0f, &Label, &MainView);
	Label.VSplitLeft(280.0f, &Label, &Dummy);
	Label.VSplitLeft(230.0f, &Label, 0);
	Dummy.VSplitLeft(170.0f, &Dummy, &SkinPrefix);
	SkinPrefix.VSplitLeft(120.0f, &SkinPrefix, &EyesRight);
	char aBuf[128 + IO_MAX_PATH_LENGTH];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Your skin"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&m_Dummy, Localize("Dummy settings"), m_Dummy, &DummyLabel))
	{
		m_Dummy ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&m_Dummy, &DummyLabel, Localize("Toggle to edit your dummy settings"));

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&g_Config.m_ClDownloadSkins, Localize("Download skins"), g_Config.m_ClDownloadSkins, &DummyLabel))
	{
		g_Config.m_ClDownloadSkins ^= 1;
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&g_Config.m_ClVanillaSkinsOnly, Localize("Vanilla skins only"), g_Config.m_ClVanillaSkinsOnly, &DummyLabel))
	{
		g_Config.m_ClVanillaSkinsOnly ^= 1;
		s_InitSkinlist = true;
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	if(DoButton_CheckBox(&g_Config.m_ClFatSkins, Localize("Fat skins (DDFat)"), g_Config.m_ClFatSkins, &DummyLabel))
	{
		g_Config.m_ClFatSkins ^= 1;
	}

	SkinPrefix.HSplitTop(20.0f, &SkinPrefixLabel, &SkinPrefix);
	UI()->DoLabelScaled(&SkinPrefixLabel, Localize("Skin prefix"), 14.0f, TEXTALIGN_LEFT);

	SkinPrefix.HSplitTop(20.0f, &SkinPrefixLabel, &SkinPrefix);
	{
		static int s_ClearButton = 0;
		static float s_Offset = 0.0f;
		UIEx()->DoClearableEditBox(g_Config.m_ClSkinPrefix, &s_ClearButton, &SkinPrefixLabel, g_Config.m_ClSkinPrefix, sizeof(g_Config.m_ClSkinPrefix), 14.0f, &s_Offset);
	}

	SkinPrefix.HSplitTop(2.0f, 0, &SkinPrefix);
	{
		static const char *s_aSkinPrefixes[] = {"kitty", "santa"};
		for(auto &pPrefix : s_aSkinPrefixes)
		{
			SkinPrefix.HSplitTop(20.0f, &Button, &SkinPrefix);
			Button.HMargin(2.0f, &Button);
			if(DoButton_Menu(&pPrefix, pPrefix, 0, &Button))
			{
				str_copy(g_Config.m_ClSkinPrefix, pPrefix, sizeof(g_Config.m_ClSkinPrefix));
			}
		}
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	MainView.HSplitTop(50.0f, &Label, &MainView);
	Label.VSplitLeft(230.0f, &Label, 0);
	CAnimState *pIdleState = CAnimState::GetIdle();
	vec2 OffsetToMid;
	RenderTools()->GetRenderTeeOffsetToRenderedTee(pIdleState, &OwnSkinInfo, OffsetToMid);
	vec2 TeeRenderPos(Label.x + 30.0f, Label.y + Label.h / 2.0f + OffsetToMid.y);
	int Emote = m_Dummy ? g_Config.m_ClDummyDefaultEyes : g_Config.m_ClPlayerDefaultEyes;
	RenderTools()->RenderTee(pIdleState, &OwnSkinInfo, Emote, vec2(1, 0), TeeRenderPos);
	Label.VSplitLeft(70.0f, 0, &Label);
	Label.HMargin(15.0f, &Label);

	// default eyes
	bool RenderEyesBelow = MainView.w < 750.0f;
	if(RenderEyesBelow)
	{
		Eyes.VSplitLeft(190.0f, 0, &Eyes);
		Eyes.HSplitTop(85.0f, 0, &Eyes);
	}
	else
	{
		Eyes = EyesRight;
		if(MainView.w < 810.0f)
			Eyes.VSplitRight(205.0f, 0, &Eyes);
		Eyes.HSplitTop(50.0f, &Eyes, 0);
	}
	Eyes.HSplitTop(120.0f, &EyesLabel, &Eyes);
	EyesLabel.VSplitLeft(20.0f, 0, &EyesLabel);
	EyesLabel.HSplitTop(50.0f, &EyesLabel, &Eyes);

	float Highlight = 0.0f;
	static int s_aEyeButtons[6];
	static int s_aEyesToolTip[6];
	for(int CurrentEyeEmote = 0; CurrentEyeEmote < 6; CurrentEyeEmote++)
	{
		EyesLabel.VSplitLeft(10.0f, 0, &EyesLabel);
		EyesLabel.VSplitLeft(50.0f, &EyesTee, &EyesLabel);

		if(CurrentEyeEmote == 2 && !RenderEyesBelow)
		{
			Eyes.HSplitTop(60.0f, &EyesLabel, 0);
			EyesLabel.HSplitTop(10.0f, 0, &EyesLabel);
		}
		Highlight = (m_Dummy) ? g_Config.m_ClDummyDefaultEyes == CurrentEyeEmote : g_Config.m_ClPlayerDefaultEyes == CurrentEyeEmote;
		if(DoButton_Menu(&s_aEyeButtons[CurrentEyeEmote], "", 0, &EyesTee, 0, CUI::CORNER_ALL, 10.0f, 0.0f, vec4(1, 1, 1, 0.5f + Highlight * 0.25f), vec4(1, 1, 1, 0.25f + Highlight * 0.25f)))
		{
			if(m_Dummy)
			{
				g_Config.m_ClDummyDefaultEyes = CurrentEyeEmote;
				if(g_Config.m_ClDummy)
					GameClient()->m_Emoticon.EyeEmote(CurrentEyeEmote);
			}
			else
			{
				g_Config.m_ClPlayerDefaultEyes = CurrentEyeEmote;
				if(!g_Config.m_ClDummy)
					GameClient()->m_Emoticon.EyeEmote(CurrentEyeEmote);
			}
		}
		GameClient()->m_Tooltips.DoToolTip(&s_aEyesToolTip[CurrentEyeEmote], &EyesTee, Localize("Choose default eyes when joining a server"));
		RenderTools()->RenderTee(pIdleState, &OwnSkinInfo, CurrentEyeEmote, vec2(1, 0), vec2(EyesTee.x + 25.0f, EyesTee.y + EyesTee.h / 2.0f + OffsetToMid.y));
	}

	static float s_OffsetSkin = 0.0f;
	static int s_ClearButton = 0;
	SUIExEditBoxProperties EditProps;
	EditProps.m_pEmptyText = "default";
	if(UIEx()->DoClearableEditBox(pSkinName, &s_ClearButton, &Label, pSkinName, sizeof(g_Config.m_ClPlayerSkin), 14.0f, &s_OffsetSkin, false, CUI::CORNER_ALL, EditProps))
	{
		SetNeedSendInfo();
	}

	// custom color selector
	MainView.HSplitTop(20.0f + RenderEyesBelow * 25.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(150.0f, &Button, 0);
	static int s_CustomColorID = 0;
	if(DoButton_CheckBox(&s_CustomColorID, Localize("Custom colors"), *UseCustomColor, &Button))
	{
		*UseCustomColor = *UseCustomColor ? 0 : 1;
		SetNeedSendInfo();
	}

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(82.5f, &Label, &MainView);
	if(*UseCustomColor)
	{
		CUIRect aRects[2];
		Label.VSplitMid(&aRects[0], &aRects[1], 20.0f);

		unsigned *paColors[2] = {ColorBody, ColorFeet};
		const char *paParts[] = {Localize("Body"), Localize("Feet")};

		for(int i = 0; i < 2; i++)
		{
			aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
			UI()->DoLabelScaled(&Label, paParts[i], 14.0f, TEXTALIGN_LEFT);
			aRects[i].VSplitLeft(10.0f, 0, &aRects[i]);
			aRects[i].HSplitTop(2.5f, 0, &aRects[i]);

			unsigned PrevColor = *paColors[i];
			RenderHSLScrollbars(&aRects[i], paColors[i], false, true);

			if(PrevColor != *paColors[i])
			{
				SetNeedSendInfo();
			}
		}
	}

	// skin selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(230.0f - RenderEyesBelow * 25.0f, &SkinList, &MainView);
	static sorted_array<CUISkin> s_paSkinList;
	static int s_SkinCount = 0;
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinlist || m_pClient->m_Skins.Num() != s_SkinCount)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_Skins.Num(); ++i)
		{
			const CSkin *s = m_pClient->m_Skins.Get(i);

			// filter quick search
			if(g_Config.m_ClSkinFilterString[0] != '\0' && !str_utf8_find_nocase(s->m_aName, g_Config.m_ClSkinFilterString))
				continue;

			// no special skins
			if((s->m_aName[0] == 'x' && s->m_aName[1] == '_'))
				continue;

			// vanilla skins only
			if(g_Config.m_ClVanillaSkinsOnly && !s->m_IsVanilla)
				continue;

			if(s == 0)
				continue;

			s_paSkinList.add(CUISkin(s));
		}
		s_InitSkinlist = false;
		s_SkinCount = m_pClient->m_Skins.Num();
	}

	int OldSelected = -1;
	UiDoListboxStart(&s_InitSkinlist, &SkinList, 50.0f, Localize("Skins"), "", s_paSkinList.size(), 4, OldSelected, s_ScrollValue);
	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CSkin *s = s_paSkinList[i].m_pSkin;

		if(str_comp(s->m_aName, pSkinName) == 0)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(s_paSkinList[i].m_pSkin, OldSelected == i);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info = OwnSkinInfo;
			Info.m_CustomColoredSkin = *UseCustomColor;

			Info.m_OriginalRenderSkin = s->m_OriginalSkin;
			Info.m_ColorableRenderSkin = s->m_ColorableSkin;
			Info.m_SkinMetrics = s->m_Metrics;

			RenderTools()->GetRenderTeeOffsetToRenderedTee(pIdleState, &Info, OffsetToMid);
			TeeRenderPos = vec2(Item.m_Rect.x + 30, Item.m_Rect.y + Item.m_Rect.h / 2 + OffsetToMid.y);
			RenderTools()->RenderTee(pIdleState, &Info, 0, vec2(1.0f, 0.0f), TeeRenderPos);

			Item.m_Rect.VSplitLeft(60.0f, 0, &Item.m_Rect);
			str_format(aBuf, sizeof(aBuf), "%s", s->m_aName);
			SLabelProperties Props;
			Props.m_MaxWidth = Item.m_Rect.w;
			RenderTools()->UI()->DoLabelScaled(&Item.m_Rect, aBuf, 12.0f, TEXTALIGN_LEFT, Props);
			if(g_Config.m_Debug)
			{
				ColorRGBA BloodColor = *UseCustomColor ? color_cast<ColorRGBA>(ColorHSLA(*ColorBody)) : s->m_BloodColor;
				Graphics()->TextureClear();
				Graphics()->QuadsBegin();
				Graphics()->SetColor(BloodColor.r, BloodColor.g, BloodColor.b, 1.0f);
				IGraphics::CQuadItem QuadItem(Item.m_Rect.x, Item.m_Rect.y, 12.0f, 12.0f);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		mem_copy(pSkinName, s_paSkinList[NewSelected].m_pSkin->m_aName, sizeof(g_Config.m_ClPlayerSkin));
		SetNeedSendInfo();
	}

	// render quick search
	{
		MainView.HSplitBottom(ms_ButtonHeight, &MainView, &QuickSearch);
		QuickSearch.VSplitLeft(240.0f, &QuickSearch, &SkinDB);
		QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
		const char *pSearchLabel = "\xEF\x80\x82";
		TextRender()->SetCurFont(TextRender()->GetFont(TEXT_FONT_ICON_FONT));
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);

		SLabelProperties Props;
		Props.m_AlignVertically = 0;
		UI()->DoLabelScaled(&QuickSearch, pSearchLabel, 14.0f, TEXTALIGN_LEFT, Props);
		float wSearch = TextRender()->TextWidth(0, 14.0f, pSearchLabel, -1, -1.0f);
		TextRender()->SetRenderFlags(0);
		TextRender()->SetCurFont(NULL);
		QuickSearch.VSplitLeft(wSearch, 0, &QuickSearch);
		QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
		QuickSearch.VSplitLeft(QuickSearch.w - 15.0f, &QuickSearch, &QuickSearchClearButton);
		static int s_ClearButtonSearch = 0;
		static float s_Offset = 0.0f;
		SUIExEditBoxProperties EditPropsSearch;
		if(Input()->KeyPress(KEY_F) && Input()->ModifierIsPressed())
		{
			UI()->SetActiveItem(&g_Config.m_ClSkinFilterString);

			EditPropsSearch.m_SelectText = true;
		}
		EditPropsSearch.m_pEmptyText = Localize("Search");
		if(UIEx()->DoClearableEditBox(&g_Config.m_ClSkinFilterString, &s_ClearButtonSearch, &QuickSearch, g_Config.m_ClSkinFilterString, sizeof(g_Config.m_ClSkinFilterString), 14.0f, &s_Offset, false, CUI::CORNER_ALL, EditPropsSearch))
			s_InitSkinlist = true;
	}

	SkinDB.VSplitLeft(150.0f, &SkinDB, &DirectoryButton);
	SkinDB.HSplitTop(5.0f, 0, &SkinDB);
	static int s_SkinDBDirID = 0;
	if(DoButton_Menu(&s_SkinDBDirID, Localize("Skin Database"), 0, &SkinDB))
	{
		if(!open_link("https://ddnet.tw/skins/"))
		{
			dbg_msg("menus", "couldn't open link");
		}
	}

	DirectoryButton.HSplitTop(5.0f, 0, &DirectoryButton);
	DirectoryButton.VSplitRight(175.0f, 0, &DirectoryButton);
	DirectoryButton.VSplitRight(25.0f, &DirectoryButton, &RefreshButton);
	DirectoryButton.VSplitRight(10.0f, &DirectoryButton, 0);
	static int s_DirectoryButtonID = 0;
	if(DoButton_Menu(&s_DirectoryButtonID, Localize("Skins directory"), 0, &DirectoryButton))
	{
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, "skins", aBuf, sizeof(aBuf));
		Storage()->CreateFolder("skins", IStorage::TYPE_SAVE);
		if(!open_file(aBuf))
		{
			dbg_msg("menus", "couldn't open file");
		}
	}

	TextRender()->SetCurFont(TextRender()->GetFont(TEXT_FONT_ICON_FONT));
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
	static int s_SkinRefreshButtonID = 0;
	if(DoButton_Menu(&s_SkinRefreshButtonID, "\xEF\x80\x9E", 0, &RefreshButton, NULL, 15, 5, 0, vec4(1.0f, 1.0f, 1.0f, 0.75f), vec4(1, 1, 1, 0.5f), 0))
	{
		// reset render flags for possible loading screen
		TextRender()->SetRenderFlags(0);
		TextRender()->SetCurFont(NULL);
		int64_t SkinStartLoadTime = time_get_microseconds();
		m_pClient->m_Skins.Refresh([&](int) {
			// if skin refreshing takes to long, swap to a loading screen
			if(time_get_microseconds() - SkinStartLoadTime > 500000)
			{
				RenderLoading(false, false);
			}
		});
		s_InitSkinlist = true;
		if(Client()->State() >= IClient::STATE_ONLINE)
		{
			m_pClient->RefindSkins();
		}
	}
	TextRender()->SetRenderFlags(0);
	TextRender()->SetCurFont(NULL);
}

typedef struct
{
	CLocConstString m_Name;
	const char *m_pCommand;
	int m_KeyId;
	int m_ModifierCombination;
} CKeyInfo;

static CKeyInfo gs_aKeys[] =
	{
		{"Move left", "+left", 0, 0}, // Localize - these strings are localized within CLocConstString
		{"Move right", "+right", 0, 0},
		{"Jump", "+jump", 0, 0},
		{"Fire", "+fire", 0, 0},
		{"Hook", "+hook", 0, 0},
		{"Hook collisions", "+showhookcoll", 0, 0},
		{"Pause", "say /pause", 0, 0},
		{"Kill", "kill", 0, 0},
		{"Zoom in", "zoom+", 0, 0},
		{"Zoom out", "zoom-", 0, 0},
		{"Default zoom", "zoom", 0, 0},
		{"Show others", "say /showothers", 0, 0},
		{"Show all", "say /showall", 0, 0},
		{"Toggle dyncam", "toggle cl_dyncam 0 1", 0, 0},
		{"Toggle ghost", "toggle cl_race_show_ghost 0 1", 0, 0},

		{"Hammer", "+weapon1", 0, 0},
		{"Pistol", "+weapon2", 0, 0},
		{"Shotgun", "+weapon3", 0, 0},
		{"Grenade", "+weapon4", 0, 0},
		{"Laser", "+weapon5", 0, 0},
		{"Next weapon", "+nextweapon", 0, 0},
		{"Prev. weapon", "+prevweapon", 0, 0},

		{"Vote yes", "vote yes", 0, 0},
		{"Vote no", "vote no", 0, 0},

		{"Chat", "+show_chat; chat all", 0, 0},
		{"Team chat", "+show_chat; chat team", 0, 0},
		{"Converse", "+show_chat; chat all /c ", 0, 0},
		{"Chat command", "+show_chat; chat all /", 0, 0},
		{"Show chat", "+show_chat", 0, 0},

		{"Toggle dummy", "toggle cl_dummy 0 1", 0, 0},
		{"Dummy copy", "toggle cl_dummy_copy_moves 0 1", 0, 0},
		{"Hammerfly dummy", "toggle cl_dummy_hammer 0 1", 0, 0},

		{"Emoticon", "+emote", 0, 0},
		{"Spectator mode", "+spectate", 0, 0},
		{"Spectate next", "spectate_next", 0, 0},
		{"Spectate previous", "spectate_previous", 0, 0},
		{"Console", "toggle_local_console", 0, 0},
		{"Remote console", "toggle_remote_console", 0, 0},
		{"Screenshot", "screenshot", 0, 0},
		{"Scoreboard", "+scoreboard", 0, 0},
		{"Statboard", "+statboard", 0, 0},
		{"Lock team", "say /lock", 0, 0},
		{"Show entities", "toggle cl_overlay_entities 0 100", 0, 0},
		{"Show HUD", "toggle cl_showhud 0 1", 0, 0},
};

/*	This is for scripts/languages to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");
	Localize("Hook collisions");Localize("Pause");Localize("Kill");Localize("Zoom in");Localize("Zoom out");
	Localize("Default zoom");Localize("Show others");Localize("Show all");Localize("Toggle dyncam");
	Localize("Toggle dummy");Localize("Toggle ghost");Localize("Dummy copy");Localize("Hammerfly dummy");
	Localize("Hammer");Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Laser");
	Localize("Next weapon");Localize("Prev. weapon");Localize("Vote yes");Localize("Vote no");
	Localize("Chat");Localize("Team chat");Localize("Converse");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");
	Localize("Remote console");Localize("Screenshot");Localize("Scoreboard");Localize("Statboard");
	Localize("Lock team");Localize("Show entities");Localize("Show HUD");Localize("Chat command");
*/

void CMenus::UiDoGetButtons(int Start, int Stop, CUIRect View, CUIRect ScopeView)
{
	for(int i = Start; i < Stop; i++)
	{
		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(20.0f, &Button, &View);
		Button.VSplitLeft(135.0f, &Label, &Button);

		if(Button.y >= ScopeView.y && Button.y + Button.h <= ScopeView.y + ScopeView.h)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s:", Localize((const char *)Key.m_Name));

			UI()->DoLabelScaled(&Label, aBuf, 13.0f, TEXTALIGN_LEFT);
			int OldId = Key.m_KeyId, OldModifierCombination = Key.m_ModifierCombination, NewModifierCombination;
			int NewId = DoKeyReader((void *)&Key.m_Name, &Button, OldId, OldModifierCombination, &NewModifierCombination);
			if(NewId != OldId || NewModifierCombination != OldModifierCombination)
			{
				if(OldId != 0 || NewId == 0)
					m_pClient->m_Binds.Bind(OldId, "", false, OldModifierCombination);
				if(NewId != 0)
					m_pClient->m_Binds.Bind(NewId, gs_aKeys[i].m_pCommand, false, NewModifierCombination);
			}
		}

		View.HSplitTop(2.0f, 0, &View);
	}
}

void CMenus::RenderSettingsControls(CUIRect MainView)
{
	char aBuf[128];

	// this is kinda slow, but whatever
	for(auto &Key : gs_aKeys)
		Key.m_KeyId = Key.m_ModifierCombination = 0;

	for(int Mod = 0; Mod < CBinds::MODIFIER_COMBINATION_COUNT; Mod++)
	{
		for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
		{
			const char *pBind = m_pClient->m_Binds.Get(KeyId, Mod);
			if(!pBind[0])
				continue;

			for(auto &Key : gs_aKeys)
				if(str_comp(pBind, Key.m_pCommand) == 0)
				{
					Key.m_KeyId = KeyId;
					Key.m_ModifierCombination = Mod;
					break;
				}
		}
	}

	// controls in a scrollable listbox
	static int s_ControlsList = 0;
	static int s_SelectedControl = -1;
	static float s_ScrollValue = 0;
	static int s_OldSelected = 0;
	// Hacky values: Size of 10.0f per item for smoother scrolling, 72 elements
	// fits the current size of controls settings
	UiDoListboxStart(&s_ControlsList, &MainView, 10.0f, Localize("Controls"), "", 72, 1, s_SelectedControl, s_ScrollValue);

	CUIRect MovementSettings, WeaponSettings, VotingSettings, ChatSettings, DummySettings, MiscSettings, ResetButton;
	CListboxItem Item = UiDoListboxNextItem(&s_OldSelected, false, false, true);
	Item.m_Rect.HSplitTop(10.0f, 0, &Item.m_Rect);
	Item.m_Rect.VSplitMid(&MovementSettings, &VotingSettings);

	// movement settings
	{
		MovementSettings.VMargin(5.0f, &MovementSettings);
		MovementSettings.HSplitTop(445.0f, &MovementSettings, &WeaponSettings);
		RenderTools()->DrawUIRect(&MovementSettings, ColorRGBA(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);
		MovementSettings.VMargin(10.0f, &MovementSettings);

		TextRender()->Text(0, MovementSettings.x, MovementSettings.y + (14.0f + 5.0f + 10.0f - 14.0f * UI()->Scale()) / 2.f, 14.0f * UI()->Scale(), Localize("Movement"), -1.0f);

		MovementSettings.HSplitTop(14.0f + 5.0f + 10.0f, 0, &MovementSettings);

		{
			CUIRect Button, Label;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(160.0f, &Label, &Button);
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Mouse sens."), g_Config.m_InpMousesens);
			UI()->DoLabel(&Label, aBuf, 14.0f * UI()->Scale(), TEXTALIGN_LEFT);
			int NewValue = (int)(UIEx()->DoScrollbarH(&g_Config.m_InpMousesens, &Button, (minimum(g_Config.m_InpMousesens, 500) - 1) / 500.0f) * 500.0f) + 1;
			if(g_Config.m_InpMousesens < 500 || NewValue < 500)
				g_Config.m_InpMousesens = minimum(NewValue, 500);
			MovementSettings.HSplitTop(20.0f, 0, &MovementSettings);
		}

		{
			CUIRect Button, Label;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(160.0f, &Label, &Button);
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("UI mouse s."), g_Config.m_UiMousesens);
			UI()->DoLabel(&Label, aBuf, 14.0f * UI()->Scale(), TEXTALIGN_LEFT);
			int NewValue = (int)(UIEx()->DoScrollbarH(&g_Config.m_UiMousesens, &Button, (minimum(g_Config.m_UiMousesens, 500) - 1) / 500.0f) * 500.0f) + 1;
			if(g_Config.m_UiMousesens < 500 || NewValue < 500)
				g_Config.m_UiMousesens = minimum(NewValue, 500);
			MovementSettings.HSplitTop(20.0f, 0, &MovementSettings);
		}

		UiDoGetButtons(0, 15, MovementSettings, MainView);
	}

	// weapon settings
	{
		WeaponSettings.HSplitTop(10.0f, 0, &WeaponSettings);
		WeaponSettings.HSplitTop(190.0f, &WeaponSettings, &ResetButton);
		RenderTools()->DrawUIRect(&WeaponSettings, ColorRGBA(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);
		WeaponSettings.VMargin(10.0f, &WeaponSettings);

		TextRender()->Text(0, WeaponSettings.x, WeaponSettings.y + (14.0f + 5.0f + 10.0f - 14.0f * UI()->Scale()) / 2.f, 14.0f * UI()->Scale(), Localize("Weapon"), -1.0f);

		WeaponSettings.HSplitTop(14.0f + 5.0f + 10.0f, 0, &WeaponSettings);
		UiDoGetButtons(15, 22, WeaponSettings, MainView);
	}

	// defaults
	{
		ResetButton.HSplitTop(10.0f, 0, &ResetButton);
		ResetButton.HSplitTop(40.0f, &ResetButton, 0);
		RenderTools()->DrawUIRect(&ResetButton, ColorRGBA(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);
		ResetButton.HMargin(10.0f, &ResetButton);
		ResetButton.VMargin(30.0f, &ResetButton);
		ResetButton.HSplitTop(20.0f, &ResetButton, 0);
		static int s_DefaultButton = 0;
		if(DoButton_Menu((void *)&s_DefaultButton, Localize("Reset to defaults"), 0, &ResetButton))
			m_pClient->m_Binds.SetDefaults();
	}

	// voting settings
	{
		VotingSettings.VMargin(5.0f, &VotingSettings);
		VotingSettings.HSplitTop(80.0f, &VotingSettings, &ChatSettings);
		RenderTools()->DrawUIRect(&VotingSettings, ColorRGBA(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);
		VotingSettings.VMargin(10.0f, &VotingSettings);

		TextRender()->Text(0, VotingSettings.x, VotingSettings.y + (14.0f + 5.0f + 10.0f - 14.0f * UI()->Scale()) / 2.f, 14.0f * UI()->Scale(), Localize("Voting"), -1.0f);

		VotingSettings.HSplitTop(14.0f + 5.0f + 10.0f, 0, &VotingSettings);
		UiDoGetButtons(22, 24, VotingSettings, MainView);
	}

	// chat settings
	{
		ChatSettings.HSplitTop(10.0f, 0, &ChatSettings);
		ChatSettings.HSplitTop(145.0f, &ChatSettings, &DummySettings);
		RenderTools()->DrawUIRect(&ChatSettings, ColorRGBA(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);
		ChatSettings.VMargin(10.0f, &ChatSettings);

		TextRender()->Text(0, ChatSettings.x, ChatSettings.y + (14.0f + 5.0f + 10.0f - 14.0f * UI()->Scale()) / 2.f, 14.0f * UI()->Scale(), Localize("Chat"), -1.0f);

		ChatSettings.HSplitTop(14.0f + 5.0f + 10.0f, 0, &ChatSettings);
		UiDoGetButtons(24, 29, ChatSettings, MainView);
	}

	// dummy settings
	{
		DummySettings.HSplitTop(10.0f, 0, &DummySettings);
		DummySettings.HSplitTop(100.0f, &DummySettings, &MiscSettings);
		RenderTools()->DrawUIRect(&DummySettings, ColorRGBA(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);
		DummySettings.VMargin(10.0f, &DummySettings);

		TextRender()->Text(0, DummySettings.x, DummySettings.y + (14.0f + 5.0f + 10.0f - 14.0f * UI()->Scale()) / 2.f, 14.0f * UI()->Scale(), Localize("Dummy"), -1.0f);

		DummySettings.HSplitTop(14.0f + 5.0f + 10.0f, 0, &DummySettings);
		UiDoGetButtons(29, 32, DummySettings, MainView);
	}

	// misc settings
	{
		MiscSettings.HSplitTop(10.0f, 0, &MiscSettings);
		MiscSettings.HSplitTop(300.0f, &MiscSettings, 0);
		RenderTools()->DrawUIRect(&MiscSettings, ColorRGBA(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);
		MiscSettings.VMargin(10.0f, &MiscSettings);

		TextRender()->Text(0, MiscSettings.x, MiscSettings.y + (14.0f + 5.0f + 10.0f - 14.0f * UI()->Scale()) / 2.f, 14.0f * UI()->Scale(), Localize("Miscellaneous"), -1.0f);

		MiscSettings.HSplitTop(14.0f + 5.0f + 10.0f, 0, &MiscSettings);
		UiDoGetButtons(32, 44, MiscSettings, MainView);
	}

	UiDoListboxEnd(&s_ScrollValue, 0);
}

int CMenus::RenderDropDown(int &CurDropDownState, CUIRect *pRect, int CurSelection, const void **pIDs, const char **pStr, int PickNum, const void *pID, float &ScrollVal)
{
	if(CurDropDownState != 0)
	{
		CUIRect ListRect;
		pRect->HSplitTop(24.0f * PickNum, &ListRect, pRect);
		char aBuf[1024];
		UiDoListboxStart(&pID, &ListRect, 24.0f, "", aBuf, PickNum, 1, CurSelection, ScrollVal);
		for(int i = 0; i < PickNum; ++i)
		{
			CListboxItem Item = UiDoListboxNextItem(pIDs[i], CurSelection == i);
			if(Item.m_Visible)
			{
				str_format(aBuf, sizeof(aBuf), "%s", pStr[i]);
				UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, TEXTALIGN_CENTER);
			}
		}
		bool ClickedItem = false;
		int NewIndex = UiDoListboxEnd(&ScrollVal, NULL, &ClickedItem);
		if(ClickedItem)
		{
			CurDropDownState = 0;
			return NewIndex;
		}
		else
			return CurSelection;
	}
	else
	{
		CUIRect Button;
		pRect->HSplitTop(24.0f, &Button, pRect);
		if(DoButton_MenuTab(pID, CurSelection > -1 ? pStr[CurSelection] : "", 0, &Button, CUI::CORNER_ALL, NULL, NULL, NULL, NULL, 4.0f))
			CurDropDownState = 1;

		CUIRect DropDownIcon = Button;
		DropDownIcon.HMargin(2.0f, &DropDownIcon);
		DropDownIcon.VSplitRight(5.0f, &DropDownIcon, nullptr);
		TextRender()->SetCurFont(TextRender()->GetFont(TEXT_FONT_ICON_FONT));
		TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		UI()->DoLabelScaled(&DropDownIcon, "\xEF\x84\xBA", DropDownIcon.h * CUI::ms_FontmodHeight, TEXTALIGN_RIGHT);
		TextRender()->SetRenderFlags(0);
		TextRender()->SetCurFont(NULL);
		return CurSelection;
	}
}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	CUIRect Button, Label;
	char aBuf[128];
	bool CheckSettings = false;

	static const int MAX_RESOLUTIONS = 256;
	static CVideoMode s_aModes[MAX_RESOLUTIONS];
	static int s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
	static int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static bool s_GfxBackendChanged = false;
	static bool s_GfxGPUChanged = false;
	static int s_GfxHighdpi = g_Config.m_GfxHighdpi;

	static int s_InitDisplayAllVideoModes = g_Config.m_GfxDisplayAllVideoModes;

	static bool s_WasInit = false;
	static bool s_ModesReload = false;
	if(!s_WasInit)
	{
		s_WasInit = true;

		Graphics()->AddWindowResizeListener([&](void *pUser) {
			s_ModesReload = true;
		},
			this);
	}

	if(s_ModesReload || g_Config.m_GfxDisplayAllVideoModes != s_InitDisplayAllVideoModes)
	{
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		s_ModesReload = false;
		s_InitDisplayAllVideoModes = g_Config.m_GfxDisplayAllVideoModes;
	}

	CUIRect ModeList, ModeLabel;
	MainView.VSplitLeft(350.0f, &MainView, &ModeList);
	ModeList.HSplitTop(24.0f, &ModeLabel, &ModeList);
	MainView.VSplitLeft(340.0f, &MainView, 0);

	// display mode list
	static float s_ScrollValue = 0;
	static const float sc_RowHeightResList = 22.0f;
	static const float sc_FontSizeResListHeader = 12.0f;
	static const float sc_FontSizeResList = 10.0f;
	int OldSelected = -1;
	{
		int G = std::gcd(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
		str_format(aBuf, sizeof(aBuf), "%s: %dx%d @%dhz %d bit (%d:%d)", Localize("Current"), int(g_Config.m_GfxScreenWidth * Graphics()->ScreenHiDPIScale()), int(g_Config.m_GfxScreenHeight * Graphics()->ScreenHiDPIScale()), g_Config.m_GfxScreenRefreshRate, g_Config.m_GfxColorDepth, g_Config.m_GfxScreenWidth / G, g_Config.m_GfxScreenHeight / G);
	}

	UI()->DoLabelScaled(&ModeLabel, aBuf, sc_FontSizeResListHeader, TEXTALIGN_CENTER);
	UiDoListboxStart(&s_NumNodes, &ModeList, sc_RowHeightResList, Localize("Display Modes"), aBuf, s_NumNodes - 1, 1, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_NumNodes; ++i)
	{
		const int Depth = s_aModes[i].m_Red + s_aModes[i].m_Green + s_aModes[i].m_Blue > 16 ? 24 : 16;
		if(g_Config.m_GfxColorDepth == Depth &&
			g_Config.m_GfxScreenWidth == s_aModes[i].m_WindowWidth &&
			g_Config.m_GfxScreenHeight == s_aModes[i].m_WindowHeight &&
			g_Config.m_GfxScreenRefreshRate == s_aModes[i].m_RefreshRate)
		{
			OldSelected = i;
		}

		CListboxItem Item = UiDoListboxNextItem(&s_aModes[i], OldSelected == i);
		if(Item.m_Visible)
		{
			int G = std::gcd(s_aModes[i].m_CanvasWidth, s_aModes[i].m_CanvasHeight);
			str_format(aBuf, sizeof(aBuf), " %dx%d @%dhz %d bit (%d:%d)", s_aModes[i].m_CanvasWidth, s_aModes[i].m_CanvasHeight, s_aModes[i].m_RefreshRate, Depth, s_aModes[i].m_CanvasWidth / G, s_aModes[i].m_CanvasHeight / G);
			UI()->DoLabelScaled(&Item.m_Rect, aBuf, sc_FontSizeResList, TEXTALIGN_LEFT);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		const int Depth = s_aModes[NewSelected].m_Red + s_aModes[NewSelected].m_Green + s_aModes[NewSelected].m_Blue > 16 ? 24 : 16;
		g_Config.m_GfxColorDepth = Depth;
		g_Config.m_GfxScreenWidth = s_aModes[NewSelected].m_WindowWidth;
		g_Config.m_GfxScreenHeight = s_aModes[NewSelected].m_WindowHeight;
		g_Config.m_GfxScreenRefreshRate = s_aModes[NewSelected].m_RefreshRate;
		Graphics()->Resize(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight, g_Config.m_GfxScreenRefreshRate);
	}

	// switches
	static float s_ScrollValueDrop = 0;
	const char *pWindowModes[] = {Localize("Windowed"), Localize("Windowed borderless"), Localize("Windowed fullscreen"), Localize("Desktop fullscreen"), Localize("Fullscreen")};
	static const int s_NumWindowMode = std::size(pWindowModes);
	static int s_aWindowModeIDs[s_NumWindowMode];
	const void *aWindowModeIDs[s_NumWindowMode];
	for(int i = 0; i < s_NumWindowMode; ++i)
		aWindowModeIDs[i] = &s_aWindowModeIDs[i];
	static int s_WindowModeDropDownState = 0;

	static int s_OldSelectedBackend = -1;
	static int s_OldSelectedGPU = -1;

	OldSelected = (g_Config.m_GfxFullscreen ? (g_Config.m_GfxFullscreen == 1 ? 4 : (g_Config.m_GfxFullscreen == 2 ? 3 : 2)) : (g_Config.m_GfxBorderless ? 1 : 0));

	const int NewWindowMode = RenderDropDown(s_WindowModeDropDownState, &MainView, OldSelected, aWindowModeIDs, pWindowModes, s_NumWindowMode, &s_NumWindowMode, s_ScrollValueDrop);
	if(OldSelected != NewWindowMode)
	{
		if(NewWindowMode == 0)
			Client()->SetWindowParams(0, false, true);
		else if(NewWindowMode == 1)
			Client()->SetWindowParams(0, true, true);
		else if(NewWindowMode == 2)
			Client()->SetWindowParams(3, false, false);
		else if(NewWindowMode == 3)
			Client()->SetWindowParams(2, false, true);
		else if(NewWindowMode == 4)
			Client()->SetWindowParams(1, false, true);
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	str_format(aBuf, sizeof(aBuf), "%s (%s)", Localize("V-Sync"), Localize("may cause delay"));
	if(DoButton_CheckBox(&g_Config.m_GfxVsync, aBuf, g_Config.m_GfxVsync, &Button))
	{
		Client()->ToggleWindowVSync();
	}

	if(Graphics()->GetNumScreens() > 1)
	{
		int NumScreens = Graphics()->GetNumScreens();
		MainView.HSplitTop(20.0f, &Button, &MainView);
		int Screen_MouseButton = DoButton_CheckBox_Number(&g_Config.m_GfxScreen, Localize("Screen"), g_Config.m_GfxScreen, &Button);
		if(Screen_MouseButton == 1) // inc
		{
			Client()->SwitchWindowScreen((g_Config.m_GfxScreen + 1) % NumScreens);
			s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		}
		else if(Screen_MouseButton == 2) // dec
		{
			Client()->SwitchWindowScreen((g_Config.m_GfxScreen - 1 + NumScreens) % NumScreens);
			s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		}
	}

	bool MultiSamplingChanged = false;
	MainView.HSplitTop(20.0f, &Button, &MainView);
	str_format(aBuf, sizeof(aBuf), "%s (%s)", Localize("FSAA samples"), Localize("may cause delay"));
	int GfxFsaaSamples_MouseButton = DoButton_CheckBox_Number(&g_Config.m_GfxFsaaSamples, aBuf, g_Config.m_GfxFsaaSamples, &Button);
	int CurFSAA = g_Config.m_GfxFsaaSamples == 0 ? 1 : g_Config.m_GfxFsaaSamples;
	if(GfxFsaaSamples_MouseButton == 1) // inc
	{
		g_Config.m_GfxFsaaSamples = std::pow(2, (int)std::log2(CurFSAA) + 1);
		if(g_Config.m_GfxFsaaSamples > 64)
			g_Config.m_GfxFsaaSamples = 0;
		MultiSamplingChanged = true;
	}
	else if(GfxFsaaSamples_MouseButton == 2) // dec
	{
		if(CurFSAA == 1)
			g_Config.m_GfxFsaaSamples = 64;
		else if(CurFSAA == 2)
			g_Config.m_GfxFsaaSamples = 0;
		else
			g_Config.m_GfxFsaaSamples = std::pow(2, (int)std::log2(CurFSAA) - 1);
		MultiSamplingChanged = true;
	}

	uint32_t MultiSamplingCountBackend = 0;
	if(MultiSamplingChanged)
	{
		if(Graphics()->SetMultiSampling(g_Config.m_GfxFsaaSamples, MultiSamplingCountBackend))
		{
			// try again with 0 if mouse click was increasing multi sampling
			// else just accept the current value as is
			if((uint32_t)g_Config.m_GfxFsaaSamples > MultiSamplingCountBackend && GfxFsaaSamples_MouseButton == 1)
				Graphics()->SetMultiSampling(0, MultiSamplingCountBackend);
			g_Config.m_GfxFsaaSamples = (int)MultiSamplingCountBackend;
		}
		else
		{
			CheckSettings = true;
		}
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button))
		g_Config.m_GfxHighDetail ^= 1;
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_GfxHighDetail, &Button, Localize("Allows maps to render with more detail"));

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxHighdpi, Localize("Use high DPI"), g_Config.m_GfxHighdpi, &Button))
	{
		CheckSettings = true;
		g_Config.m_GfxHighdpi ^= 1;
	}

	MainView.HSplitTop(20.0f, &Label, &MainView);
	Label.VSplitLeft(160.0f, &Label, &Button);
	if(g_Config.m_GfxRefreshRate)
		str_format(aBuf, sizeof(aBuf), "%s: %i Hz", Localize("Refresh Rate"), g_Config.m_GfxRefreshRate);
	else
		str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Refresh Rate"), "∞");
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);
	int NewRefreshRate = static_cast<int>(UIEx()->DoScrollbarH(&g_Config.m_GfxRefreshRate, &Button, (minimum(g_Config.m_GfxRefreshRate, 1000)) / 1000.0f) * 1000.0f + 0.1f);
	if(g_Config.m_GfxRefreshRate <= 1000 || NewRefreshRate < 1000)
		g_Config.m_GfxRefreshRate = NewRefreshRate;

	CUIRect Text;
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	//text.VSplitLeft(15.0f, 0, &text);
	UI()->DoLabelScaled(&Text, Localize("UI Color"), 14.0f, TEXTALIGN_LEFT);
	CUIRect HSLBar = MainView;
	RenderHSLScrollbars(&HSLBar, &g_Config.m_UiColor, true);
	MainView.y = HSLBar.y;
	MainView.h = MainView.h - MainView.y;

	// Backend list
	struct SMenuBackendInfo
	{
		int m_Major = 0;
		int m_Minor = 0;
		int m_Patch = 0;
		const char *m_pBackendName = "";
		bool m_Found = false;
	};
	std::array<std::array<SMenuBackendInfo, EGraphicsDriverAgeType::GRAPHICS_DRIVER_AGE_TYPE_COUNT>, EBackendType::BACKEND_TYPE_COUNT> aaSupportedBackends{};
	uint32_t FoundBackendCount = 0;
	for(uint32_t i = 0; i < BACKEND_TYPE_COUNT; ++i)
	{
		if(EBackendType(i) == BACKEND_TYPE_AUTO)
			continue;
		for(uint32_t n = 0; n < GRAPHICS_DRIVER_AGE_TYPE_COUNT; ++n)
		{
			auto &Info = aaSupportedBackends[i][n];
			if(Graphics()->GetDriverVersion(EGraphicsDriverAgeType(n), Info.m_Major, Info.m_Minor, Info.m_Patch, Info.m_pBackendName, EBackendType(i)))
			{
				// don't count blocked opengl drivers
				if(EBackendType(i) != BACKEND_TYPE_OPENGL || EGraphicsDriverAgeType(n) == GRAPHICS_DRIVER_AGE_TYPE_LEGACY || g_Config.m_GfxDriverIsBlocked == 0)
				{
					Info.m_Found = true;
					++FoundBackendCount;
				}
			}
		}
	}

	if(FoundBackendCount > 1)
	{
		MainView.HSplitTop(10.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Text, &MainView);
		UI()->DoLabelScaled(&Text, Localize("Renderer"), 16.0f, TEXTALIGN_CENTER);

		static float s_ScrollValueDropBackend = 0;
		static int s_BackendDropDownState = 0;

		static std::vector<std::unique_ptr<int>> vBackendIDs;
		static std::vector<const void *> vBackendIDPtrs;
		static std::vector<std::string> vBackendIDNames;
		static std::vector<const char *> vBackendIDNamesCStr;
		static std::vector<SMenuBackendInfo> vBackendInfos;

		size_t BackendCount = FoundBackendCount + 1;
		vBackendIDs.resize(BackendCount);
		vBackendIDPtrs.resize(BackendCount);
		vBackendIDNames.resize(BackendCount);
		vBackendIDNamesCStr.resize(BackendCount);
		vBackendInfos.resize(BackendCount);

		char aTmpBackendName[256];

		auto IsInfoDefault = [](const SMenuBackendInfo &CheckInfo) {
			return str_comp_nocase(CheckInfo.m_pBackendName, "OpenGL") == 0 && CheckInfo.m_Major == 3 && CheckInfo.m_Minor == 0 && CheckInfo.m_Patch == 0;
		};

		int OldSelectedBackend = -1;
		uint32_t CurCounter = 0;
		for(uint32_t i = 0; i < BACKEND_TYPE_COUNT; ++i)
		{
			for(uint32_t n = 0; n < GRAPHICS_DRIVER_AGE_TYPE_COUNT; ++n)
			{
				auto &Info = aaSupportedBackends[i][n];
				if(Info.m_Found)
				{
					if(vBackendIDs[CurCounter].get() == nullptr)
						vBackendIDs[CurCounter] = std::make_unique<int>();
					vBackendIDPtrs[CurCounter] = vBackendIDs[CurCounter].get();

					{
						bool IsDefault = IsInfoDefault(Info);
						str_format(aTmpBackendName, sizeof(aTmpBackendName), "%s (%d.%d.%d)%s%s", Info.m_pBackendName, Info.m_Major, Info.m_Minor, Info.m_Patch, IsDefault ? " - " : "", IsDefault ? Localize("default") : "");
						vBackendIDNames[CurCounter] = aTmpBackendName;
						vBackendIDNamesCStr[CurCounter] = vBackendIDNames[CurCounter].c_str();
						if(str_comp_nocase(Info.m_pBackendName, g_Config.m_GfxBackend) == 0 && g_Config.m_GfxGLMajor == Info.m_Major && g_Config.m_GfxGLMinor == Info.m_Minor && g_Config.m_GfxGLPatch == Info.m_Patch)
						{
							OldSelectedBackend = CurCounter;
						}

						vBackendInfos[CurCounter] = Info;
					}
					++CurCounter;
				}
			}
		}

		if(OldSelectedBackend != -1)
		{
			// no custom selected
			BackendCount -= 1;
		}
		else
		{
			// custom selected one
			if(vBackendIDs[CurCounter].get() == nullptr)
				vBackendIDs[CurCounter] = std::make_unique<int>();
			vBackendIDPtrs[CurCounter] = vBackendIDs[CurCounter].get();

			str_format(aTmpBackendName, sizeof(aTmpBackendName), "%s (%s %d.%d.%d)", Localize("custom"), g_Config.m_GfxBackend, g_Config.m_GfxGLMajor, g_Config.m_GfxGLMinor, g_Config.m_GfxGLPatch);
			vBackendIDNames[CurCounter] = aTmpBackendName;
			vBackendIDNamesCStr[CurCounter] = vBackendIDNames[CurCounter].c_str();
			OldSelectedBackend = CurCounter;

			vBackendInfos[CurCounter].m_pBackendName = "custom";
			vBackendInfos[CurCounter].m_Major = g_Config.m_GfxGLMajor;
			vBackendInfos[CurCounter].m_Minor = g_Config.m_GfxGLMinor;
			vBackendInfos[CurCounter].m_Patch = g_Config.m_GfxGLPatch;
		}

		if(s_OldSelectedBackend == -1)
			s_OldSelectedBackend = OldSelectedBackend;

		static int s_BackendCount = 0;
		s_BackendCount = BackendCount;

		const int NewBackend = RenderDropDown(s_BackendDropDownState, &MainView, OldSelectedBackend, vBackendIDPtrs.data(), vBackendIDNamesCStr.data(), s_BackendCount, &s_BackendCount, s_ScrollValueDropBackend);
		if(OldSelectedBackend != NewBackend)
		{
			str_copy(g_Config.m_GfxBackend, vBackendInfos[NewBackend].m_pBackendName, sizeof(g_Config.m_GfxBackend));
			g_Config.m_GfxGLMajor = vBackendInfos[NewBackend].m_Major;
			g_Config.m_GfxGLMinor = vBackendInfos[NewBackend].m_Minor;
			g_Config.m_GfxGLPatch = vBackendInfos[NewBackend].m_Patch;

			CheckSettings = true;
			s_GfxBackendChanged = s_OldSelectedBackend != NewBackend;
		}
	}

	// GPU list
	const auto &GPUList = Graphics()->GetGPUs();
	if(GPUList.m_GPUs.size() > 1)
	{
		MainView.HSplitTop(10.0f, nullptr, &MainView);
		MainView.HSplitTop(20.0f, &Text, &MainView);
		UI()->DoLabelScaled(&Text, Localize("Graphics cards"), 16.0f, TEXTALIGN_CENTER);

		static float s_ScrollValueDropGPU = 0;
		static int s_GPUDropDownState = 0;

		static std::vector<std::unique_ptr<int>> vGPUIDs;
		static std::vector<const void *> vGPUIDPtrs;
		static std::vector<const char *> vGPUIDNames;

		size_t GPUCount = GPUList.m_GPUs.size() + 1;
		vGPUIDs.resize(GPUCount);
		vGPUIDPtrs.resize(GPUCount);
		vGPUIDNames.resize(GPUCount);

		char aCurDeviceName[256 + 4];

		int OldSelectedGPU = -1;
		for(size_t i = 0; i < GPUCount; ++i)
		{
			if(vGPUIDs[i].get() == nullptr)
				vGPUIDs[i] = std::make_unique<int>();
			vGPUIDPtrs[i] = vGPUIDs[i].get();
			if(i == 0)
			{
				str_format(aCurDeviceName, sizeof(aCurDeviceName), "%s(%s)", Localize("auto"), GPUList.m_AutoGPU.m_Name);
				vGPUIDNames[i] = aCurDeviceName;
				if(str_comp("auto", g_Config.m_GfxGPUName) == 0)
				{
					OldSelectedGPU = 0;
				}
			}
			else
			{
				vGPUIDNames[i] = GPUList.m_GPUs[i - 1].m_Name;
				if(str_comp(GPUList.m_GPUs[i - 1].m_Name, g_Config.m_GfxGPUName) == 0)
				{
					OldSelectedGPU = i;
				}
			}
		}

		static int s_GPUCount = 0;
		s_GPUCount = GPUCount;

		if(s_OldSelectedGPU == -1)
			s_OldSelectedGPU = OldSelectedGPU;

		const int NewGPU = RenderDropDown(s_GPUDropDownState, &MainView, OldSelectedGPU, vGPUIDPtrs.data(), vGPUIDNames.data(), s_GPUCount, &s_GPUCount, s_ScrollValueDropGPU);
		if(OldSelectedGPU != NewGPU)
		{
			if(NewGPU == 0)
				str_copy(g_Config.m_GfxGPUName, "auto", sizeof(g_Config.m_GfxGPUName));
			else
				str_copy(g_Config.m_GfxGPUName, GPUList.m_GPUs[NewGPU - 1].m_Name, sizeof(g_Config.m_GfxGPUName));
			CheckSettings = true;
			s_GfxGPUChanged = NewGPU != s_OldSelectedGPU;
		}
	}

	// check if the new settings require a restart
	if(CheckSettings)
	{
		m_NeedRestartGraphics = !(s_GfxFsaaSamples == g_Config.m_GfxFsaaSamples &&
					  !s_GfxBackendChanged &&
					  !s_GfxGPUChanged &&
					  s_GfxHighdpi == g_Config.m_GfxHighdpi);
	}
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CUIRect Button, Label;
	static int s_SndEnable = g_Config.m_SndEnable;
	static int s_SndRate = g_Config.m_SndRate;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndEnable, Localize("Use sounds"), g_Config.m_SndEnable, &Button))
	{
		g_Config.m_SndEnable ^= 1;
		if(g_Config.m_SndEnable)
		{
			if(g_Config.m_SndMusic && Client()->State() == IClient::STATE_OFFLINE)
				m_pClient->m_Sounds.Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
		}
		else
			m_pClient->m_Sounds.Stop(SOUND_MENU);
		m_NeedRestartSound = g_Config.m_SndEnable && (!s_SndEnable || s_SndRate != g_Config.m_SndRate);
	}

	if(!g_Config.m_SndEnable)
		return;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndMusic, Localize("Play background music"), g_Config.m_SndMusic, &Button))
	{
		g_Config.m_SndMusic ^= 1;
		if(Client()->State() == IClient::STATE_OFFLINE)
		{
			if(g_Config.m_SndMusic)
				m_pClient->m_Sounds.Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
			else
				m_pClient->m_Sounds.Stop(SOUND_MENU);
		}
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
		g_Config.m_SndNonactiveMute ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndGame, Localize("Enable game sounds"), g_Config.m_SndGame, &Button))
		g_Config.m_SndGame ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndGun, Localize("Enable gun sound"), g_Config.m_SndGun, &Button))
		g_Config.m_SndGun ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndLongPain, Localize("Enable long pain sound (used when shooting in freeze)"), g_Config.m_SndLongPain, &Button))
		g_Config.m_SndLongPain ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndServerMessage, Localize("Enable server message sound"), g_Config.m_SndServerMessage, &Button))
		g_Config.m_SndServerMessage ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndChat, Localize("Enable regular chat sound"), g_Config.m_SndChat, &Button))
		g_Config.m_SndChat ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndTeamChat, Localize("Enable team chat sound"), g_Config.m_SndTeamChat, &Button))
		g_Config.m_SndTeamChat ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndHighlight, Localize("Enable highlighted chat sound"), g_Config.m_SndHighlight, &Button))
		g_Config.m_SndHighlight ^= 1;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_ClThreadsoundloading, Localize("Threaded sound loading"), g_Config.m_ClThreadsoundloading, &Button))
		g_Config.m_ClThreadsoundloading ^= 1;

	// sample rate box
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_SndRate);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoLabelScaled(&Button, Localize("Sample rate"), 14.0f, TEXTALIGN_LEFT);
		Button.VSplitLeft(190.0f, 0, &Button);
		static float s_Offset = 0.0f;
		UIEx()->DoEditBox(&g_Config.m_SndRate, &Button, aBuf, sizeof(aBuf), 14.0f, &s_Offset);
		g_Config.m_SndRate = maximum(1, str_toint(aBuf));
		m_NeedRestartSound = !s_SndEnable || s_SndRate != g_Config.m_SndRate;
	}

	// volume slider
	{
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(190.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sound volume"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_SndVolume = (int)(UIEx()->DoScrollbarH(&g_Config.m_SndVolume, &Button, g_Config.m_SndVolume / 100.0f) * 100.0f);
	}

	// volume slider game sounds
	{
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(190.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Game sound volume"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_SndGameSoundVolume = (int)(UIEx()->DoScrollbarH(&g_Config.m_SndGameSoundVolume, &Button, g_Config.m_SndGameSoundVolume / 100.0f) * 100.0f);
	}

	// volume slider gui sounds
	{
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(190.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Chat sound volume"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_SndChatSoundVolume = (int)(UIEx()->DoScrollbarH(&g_Config.m_SndChatSoundVolume, &Button, g_Config.m_SndChatSoundVolume / 100.0f) * 100.0f);
	}

	// volume slider map sounds
	{
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(190.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Map sound volume"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_SndMapSoundVolume = (int)(UIEx()->DoScrollbarH(&g_Config.m_SndMapSoundVolume, &Button, g_Config.m_SndMapSoundVolume / 100.0f) * 100.0f);
	}

	// volume slider background music
	{
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(190.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Background music volume"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_SndBackgroundMusicVolume = (int)(UIEx()->DoScrollbarH(&g_Config.m_SndBackgroundMusicVolume, &Button, g_Config.m_SndBackgroundMusicVolume / 100.0f) * 100.0f);
	}
}

class CLanguage
{
public:
	CLanguage() = default;
	CLanguage(const char *n, const char *f, int Code) :
		m_Name(n), m_FileName(f), m_CountryCode(Code) {}

	string m_Name;
	string m_FileName;
	int m_CountryCode;

	bool operator<(const CLanguage &Other) const { return m_Name < Other.m_Name; }
};

void LoadLanguageIndexfile(IStorage *pStorage, IConsole *pConsole, sorted_array<CLanguage> *pLanguages)
{
	IOHANDLE File = pStorage->OpenFile("languages/index.txt", IOFLAG_READ | IOFLAG_SKIP_BOM, IStorage::TYPE_ALL);
	if(!File)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "couldn't open index file");
		return;
	}

	char aOrigin[128];
	char aReplacement[128];
	CLineReader LineReader;
	LineReader.Init(File);
	char *pLine;
	while((pLine = LineReader.Get()))
	{
		if(!str_length(pLine) || pLine[0] == '#') // skip empty lines and comments
			continue;

		str_copy(aOrigin, pLine, sizeof(aOrigin));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			(void)LineReader.Get();
			continue;
		}
		str_copy(aReplacement, pLine + 3, sizeof(aReplacement));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			continue;
		}

		char aFileName[IO_MAX_PATH_LENGTH];
		str_format(aFileName, sizeof(aFileName), "languages/%s.txt", aOrigin);
		pLanguages->add(CLanguage(aReplacement, aFileName, str_toint(pLine + 3)));
	}
	io_close(File);
}

void CMenus::RenderLanguageSelection(CUIRect MainView)
{
	static int s_LanguageList = 0;
	static int s_SelectedLanguage = 0;
	static sorted_array<CLanguage> s_Languages;
	static float s_ScrollValue = 0;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(CLanguage("English", "", 826));
		LoadLanguageIndexfile(Storage(), Console(), &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
			if(str_comp(s_Languages[i].m_FileName, g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
	}

	int OldSelected = s_SelectedLanguage;

	UiDoListboxStart(&s_LanguageList, &MainView, 24.0f, Localize("Language"), "", s_Languages.size(), 1, s_SelectedLanguage, s_ScrollValue);

	for(sorted_array<CLanguage>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem(&r.front());

		if(Item.m_Visible)
		{
			CUIRect Rect;
			Item.m_Rect.VSplitLeft(Item.m_Rect.h * 2.0f, &Rect, &Item.m_Rect);
			Rect.VMargin(6.0f, &Rect);
			Rect.HMargin(3.0f, &Rect);
			ColorRGBA Color(1.0f, 1.0f, 1.0f, 1.0f);
			m_pClient->m_CountryFlags.Render(r.front().m_CountryCode, &Color, Rect.x, Rect.y, Rect.w, Rect.h);
			Item.m_Rect.HSplitTop(2.0f, 0, &Item.m_Rect);
			UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, 16.0f, TEXTALIGN_LEFT);
		}
	}

	s_SelectedLanguage = UiDoListboxEnd(&s_ScrollValue, 0);

	if(OldSelected != s_SelectedLanguage)
	{
		str_copy(g_Config.m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(g_Config.m_ClLanguagefile));
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName, Storage(), Console());
		GameClient()->OnLanguageChange();
	}
}

void CMenus::RenderSettings(CUIRect MainView)
{
	// render background
	CUIRect Temp, TabBar, RestartWarning;
	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);
	MainView.HSplitBottom(15.0f, &MainView, &RestartWarning);
	TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_BR, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);

	CUIRect Button;

	const char *aTabs[] = {
		Localize("Language"),
		Localize("General"),
		Localize("Player"),
		("Tee"),
		Localize("HUD"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound"),
		Localize("DDNet"),
		Localize("Assets")};

	int NumTabs = (int)std::size(aTabs);
	int PreviousPage = g_Config.m_UiSettingsPage;

	for(int i = 0; i < NumTabs; i++)
	{
		TabBar.HSplitTop(10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(DoButton_MenuTab(aTabs[i], aTabs[i], g_Config.m_UiSettingsPage == i, &Button, CUI::CORNER_R, &m_aAnimatorsSettingsTab[i]))
			g_Config.m_UiSettingsPage = i;
	}

	if(PreviousPage != g_Config.m_UiSettingsPage)
		ms_ColorPicker.m_Active = false;

	MainView.Margin(10.0f, &MainView);

	if(g_Config.m_UiSettingsPage == SETTINGS_LANGUAGE)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_LANGUAGE);
		RenderLanguageSelection(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_GENERAL);
		RenderSettingsGeneral(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_PLAYER);
		RenderSettingsPlayer(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_TEE);
		RenderSettingsTee(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_HUD)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_HUD);
		RenderSettingsHUD(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_CONTROLS);
		RenderSettingsControls(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_GRAPHICS);
		RenderSettingsGraphics(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_SOUND);
		RenderSettingsSound(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_DDNET)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_DDNET);
		RenderSettingsDDNet(MainView);
	}
	else if(g_Config.m_UiSettingsPage == SETTINGS_ASSETS)
	{
		m_pBackground->ChangePosition(CMenuBackground::POS_SETTINGS_ASSETS);
		RenderSettingsCustom(MainView);
	}

	if(m_NeedRestartUpdate)
	{
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		UI()->DoLabelScaled(&RestartWarning, Localize("DDNet Client needs to be restarted to complete update!"), 14.0f, TEXTALIGN_LEFT);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if(m_NeedRestartGeneral || m_NeedRestartSkins || m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartDDNet)
		UI()->DoLabelScaled(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 14.0f, TEXTALIGN_LEFT);

	RenderColorPicker();
}

ColorHSLA CMenus::RenderHSLColorPicker(const CUIRect *pRect, unsigned int *pColor, bool Alpha)
{
	ColorHSLA HSLColor(*pColor, false);
	ColorRGBA RGBColor = color_cast<ColorRGBA>(HSLColor);

	ColorRGBA Outline(1, 1, 1, 0.25f);
	const float OutlineSize = 3.0f;
	Outline.a *= UI()->ButtonColorMul(pColor);

	CUIRect Rect;
	pRect->Margin(OutlineSize, &Rect);

	RenderTools()->DrawUIRect(pRect, Outline, CUI::CORNER_ALL, 4.0f);
	RenderTools()->DrawUIRect(&Rect, RGBColor, CUI::CORNER_ALL, 4.0f);

	if(UI()->DoButtonLogic(pColor, 0, pRect))
	{
		if(ms_ColorPicker.m_Active)
		{
			CUIRect PickerRect;
			PickerRect.x = ms_ColorPicker.m_X;
			PickerRect.y = ms_ColorPicker.m_Y;
			PickerRect.w = ms_ColorPicker.ms_Width;
			PickerRect.h = ms_ColorPicker.ms_Height;

			if(ms_ColorPicker.m_pColor == pColor || UI()->MouseInside(&PickerRect))
				return HSLColor;
		}

		CUIRect *pScreen = UI()->Screen();
		ms_ColorPicker.m_X = minimum(UI()->MouseX(), pScreen->w - ms_ColorPicker.ms_Width);
		ms_ColorPicker.m_Y = minimum(UI()->MouseY(), pScreen->h - ms_ColorPicker.ms_Height);
		ms_ColorPicker.m_pColor = pColor;
		ms_ColorPicker.m_Active = true;
		ms_ColorPicker.m_AttachedRect = *pRect;
		ms_ColorPicker.m_HSVColor = color_cast<ColorHSVA, ColorHSLA>(HSLColor).Pack(false);
	}

	return HSLColor;
}

ColorHSLA CMenus::RenderHSLScrollbars(CUIRect *pRect, unsigned int *pColor, bool Alpha, bool ClampedLight)
{
	ColorHSLA Color(*pColor, Alpha);
	CUIRect Preview, Button, Label;
	char aBuf[32];
	float *paComponent[] = {&Color.h, &Color.s, &Color.l, &Color.a};
	const char *aLabels[] = {Localize("Hue"), Localize("Sat."), Localize("Lht."), Localize("Alpha")};

	float SizePerEntry = 20.0f;
	float MarginPerEntry = 5.0f;

	float OffY = (SizePerEntry + MarginPerEntry) * (3 + (Alpha ? 1 : 0)) - 40.0f;
	pRect->VSplitLeft(40.0f, &Preview, pRect);
	Preview.HSplitTop(OffY / 2.0f, NULL, &Preview);
	Preview.HSplitTop(40.0f, &Preview, NULL);

	Graphics()->TextureClear();
	{
		const float SizeBorder = 5.0f;
		Graphics()->SetColor(ColorRGBA(0.15f, 0.15f, 0.15f, 1));
		int TmpCont = RenderTools()->CreateRoundRectQuadContainer(Preview.x - SizeBorder / 2.0f, Preview.y - SizeBorder / 2.0f, Preview.w + SizeBorder, Preview.h + SizeBorder, 4.0f + SizeBorder / 2.0f, CUI::CORNER_ALL);
		Graphics()->RenderQuadContainer(TmpCont, -1);
		Graphics()->DeleteQuadContainer(TmpCont);
	}
	ColorHSLA RenderColorHSLA(Color.r, Color.g, Color.b, Color.a);
	if(ClampedLight)
		RenderColorHSLA = RenderColorHSLA.UnclampLighting();
	Graphics()->SetColor(color_cast<ColorRGBA>(RenderColorHSLA));
	int TmpCont = RenderTools()->CreateRoundRectQuadContainer(Preview.x, Preview.y, Preview.w, Preview.h, 4.0f, CUI::CORNER_ALL);
	Graphics()->RenderQuadContainer(TmpCont, -1);
	Graphics()->DeleteQuadContainer(TmpCont);

	auto &&RenderHSLColorsRect = [&](CUIRect *pColorRect) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w / 6;

		// red to yellow
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 1, 0, 0, 1),
				IGraphics::CColorVertex(1, 1, 1, 0, 1),
				IGraphics::CColorVertex(2, 1, 0, 0, 1),
				IGraphics::CColorVertex(3, 1, 1, 0, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		// yellow to green
		CurXOff += SizeColor;
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 1, 1, 0, 1),
				IGraphics::CColorVertex(1, 0, 1, 0, 1),
				IGraphics::CColorVertex(2, 1, 1, 0, 1),
				IGraphics::CColorVertex(3, 0, 1, 0, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// green to turquoise
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 0, 1, 0, 1),
				IGraphics::CColorVertex(1, 0, 1, 1, 1),
				IGraphics::CColorVertex(2, 0, 1, 0, 1),
				IGraphics::CColorVertex(3, 0, 1, 1, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// turquoise to blue
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 0, 1, 1, 1),
				IGraphics::CColorVertex(1, 0, 0, 1, 1),
				IGraphics::CColorVertex(2, 0, 1, 1, 1),
				IGraphics::CColorVertex(3, 0, 0, 1, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// blue to purple
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 0, 0, 1, 1),
				IGraphics::CColorVertex(1, 1, 0, 1, 1),
				IGraphics::CColorVertex(2, 0, 0, 1, 1),
				IGraphics::CColorVertex(3, 1, 0, 1, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		CurXOff += SizeColor;
		// purple to red
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, 1, 0, 1, 1),
				IGraphics::CColorVertex(1, 1, 0, 0, 1),
				IGraphics::CColorVertex(2, 1, 0, 1, 1),
				IGraphics::CColorVertex(3, 1, 0, 0, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	auto &&RenderHSLSatRect = [&](CUIRect *pColorRect, ColorRGBA &CurColor) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w;

		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColor);
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColor);

		LeftColor.g = 0;
		RightColor.g = 1;

		ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

		// saturation
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	auto &&RenderHSLLightRect = [&](CUIRect *pColorRect, ColorRGBA &CurColorSat) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w / (ClampedLight ? 1.0f : 2.0f);

		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColorSat);
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColorSat);

		LeftColor.b = ColorHSLA::DARKEST_LGT;
		RightColor.b = 1;

		ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

		if(!ClampedLight)
			CurXOff += SizeColor;

		// light
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		if(!ClampedLight)
		{
			CurXOff -= SizeColor;
			LeftColor.b = 0;
			RightColor.b = ColorHSLA::DARKEST_LGT;

			RightColorRGBA = color_cast<ColorRGBA>(RightColor);
			LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, 1),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, 1)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	auto &&RenderHSLAlphaRect = [&](CUIRect *pColorRect, ColorRGBA &CurColorFull) {
		Graphics()->TextureClear();
		Graphics()->TrianglesBegin();

		float CurXOff = pColorRect->x;
		float SizeColor = pColorRect->w;

		ColorHSLA RightColor = color_cast<ColorHSLA>(CurColorFull);
		ColorHSLA LeftColor = color_cast<ColorHSLA>(CurColorFull);

		LeftColor.a = 0;
		RightColor.a = 1;

		ColorRGBA RightColorRGBA = color_cast<ColorRGBA>(RightColor);
		ColorRGBA LeftColorRGBA = color_cast<ColorRGBA>(LeftColor);

		// alpha
		{
			IGraphics::CColorVertex Array[4] = {
				IGraphics::CColorVertex(0, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, LeftColorRGBA.a),
				IGraphics::CColorVertex(1, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, RightColorRGBA.a),
				IGraphics::CColorVertex(2, LeftColorRGBA.r, LeftColorRGBA.g, LeftColorRGBA.b, LeftColorRGBA.a),
				IGraphics::CColorVertex(3, RightColorRGBA.r, RightColorRGBA.g, RightColorRGBA.b, RightColorRGBA.a)};
			Graphics()->SetColorVertex(Array, 4);

			IGraphics::CFreeformItem Freeform(
				CurXOff, pColorRect->y,
				CurXOff + SizeColor, pColorRect->y,
				CurXOff, pColorRect->y + pColorRect->h,
				CurXOff + SizeColor, pColorRect->y + pColorRect->h);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}

		Graphics()->TrianglesEnd();
	};

	for(int i = 0; i < 3 + Alpha; i++)
	{
		pRect->HSplitTop(SizePerEntry, &Button, pRect);
		pRect->HSplitTop(MarginPerEntry, NULL, pRect);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);

		RenderTools()->DrawUIRect(&Button, ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f), CUI::CORNER_ALL, 1.0f);

		CUIRect Rail;
		Button.Margin(2.0f, &Rail);

		str_format(aBuf, sizeof(aBuf), "%s: %03d", aLabels[i], (int)(*paComponent[i] * 255));
		UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);

		ColorHSLA CurColorPureHSLA(RenderColorHSLA.r, 1, 0.5f, 1);
		ColorRGBA CurColorPure = color_cast<ColorRGBA>(CurColorPureHSLA);
		ColorRGBA ColorInner(1, 1, 1, 0.25f);

		if(i == 0)
		{
			ColorInner = CurColorPure;
			RenderHSLColorsRect(&Rail);
		}
		else if(i == 1)
		{
			RenderHSLSatRect(&Rail, CurColorPure);
			ColorInner = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *paComponent[1], CurColorPureHSLA.b, 1));
		}
		else if(i == 2)
		{
			ColorRGBA CurColorSat = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *paComponent[1], 0.5f, 1));
			RenderHSLLightRect(&Rail, CurColorSat);
			float LightVal = *paComponent[2];
			if(ClampedLight)
				LightVal = ColorHSLA::DARKEST_LGT + LightVal * (1.0f - ColorHSLA::DARKEST_LGT);
			ColorInner = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *paComponent[1], LightVal, 1));
		}
		else if(i == 3)
		{
			ColorRGBA CurColorFull = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *paComponent[1], *paComponent[2], 1));
			RenderHSLAlphaRect(&Rail, CurColorFull);
			float LightVal = *paComponent[2];
			if(ClampedLight)
				LightVal = ColorHSLA::DARKEST_LGT + LightVal * (1.0f - ColorHSLA::DARKEST_LGT);
			ColorInner = color_cast<ColorRGBA>(ColorHSLA(CurColorPureHSLA.r, *paComponent[1], LightVal, *paComponent[3]));
		}

		*paComponent[i] = UIEx()->DoScrollbarH(&((char *)pColor)[i], &Button, *paComponent[i], &ColorInner);
	}

	*pColor = Color.Pack(Alpha);
	return Color;
}

void CMenus::RenderSettingsHUD(CUIRect MainView)
{
	static int s_CurTab = 0;

	CUIRect TabLabel1, TabLabel2, Column,
		Section, SectionTwo;

	MainView.HSplitTop(20, &TabLabel1, &MainView);
	TabLabel1.VSplitMid(&TabLabel1, &TabLabel2);

	static int s_aPageTabs[2] = {};

	if(DoButton_MenuTab((void *)&s_aPageTabs[0], Localize("General"), s_CurTab == 0, &TabLabel1, 5, NULL, NULL, NULL, NULL, 4))
		s_CurTab = 0;
	if(DoButton_MenuTab((void *)&s_aPageTabs[1], Localize("Chat"), s_CurTab == 1, &TabLabel2, 10, NULL, NULL, NULL, NULL, 4))
		s_CurTab = 1;

	MainView.HSplitTop(10.0f, 0x0, &MainView);

	const float LineMargin = 20.0f;

	if(s_CurTab == 0)
	{ // ***** GENERAL TAB ***** //

		MainView.VSplitLeft(MainView.w * 0.55f, &MainView, &Column);

		MainView.HSplitTop(30.0f, &Section, &MainView);
		UI()->DoLabelScaled(&Section, Localize("HUD"), 20.0f, TEXTALIGN_LEFT);
		MainView.VSplitLeft(5.0f, 0x0, &MainView);
		MainView.HSplitTop(5.0f, 0x0, &MainView);

		// ***** HUD ***** //

		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhud, Localize("Show ingame HUD"), &g_Config.m_ClShowhud, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClDDRaceScoreBoard, Localize("Use DDRace Scoreboard"), &g_Config.m_ClDDRaceScoreBoard, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowIDs, Localize("Show client IDs"), &g_Config.m_ClShowIDs, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudScore, Localize("Show score"), &g_Config.m_ClShowhudScore, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowhudHealthAmmo, Localize("Show health + ammo"), &g_Config.m_ClShowhudHealthAmmo, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowChat, Localize("Show chat"), &g_Config.m_ClShowChat, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClChatTeamColors, Localize("Show names in chat in team colors"), &g_Config.m_ClChatTeamColors, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowVotesAfterVoting, Localize("Show votes window after voting"), &g_Config.m_ClShowVotesAfterVoting, &MainView, LineMargin);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClShowKillMessages, Localize("Show kill messages"), &g_Config.m_ClShowKillMessages, &MainView, LineMargin);

		MainView.HSplitTop(60.0f, 0x0, &MainView);

		// ***** Kill Messages ***** //

		MainView.HSplitTop(30.0f, &Section, &MainView);
		UI()->DoLabelScaled(&Section, Localize("Kill Messages"), 20.0f, TEXTALIGN_LEFT);
		MainView.VSplitLeft(5.0f, 0x0, &MainView);

		MainView.HSplitTop(10.0f, 0x0, &MainView);

		static int KillMessageNormalColorID;
		DoLine_ColorPicker(&KillMessageNormalColorID, 25.0f, 250.0f, 13.0f, 5.0f, &MainView, Localize("Kill Message Normal Color"), &g_Config.m_ClKillMessageNormalColor, ColorRGBA(1.0f, 1.0f, 1.0f), false);

		static int KillMessageHighlightColorID;
		DoLine_ColorPicker(&KillMessageHighlightColorID, 25.0f, 250.0f, 13.0f, 5.0f, &MainView, Localize("Kill Message Highlight Color"), &g_Config.m_ClKillMessageHighlightColor, ColorRGBA(1.0f, 1.0f, 1.0f), false);

		// ***** Laser ***** //

		MainView = Column;

		MainView.HSplitTop(30.0f, &Section, &MainView);
		UI()->DoLabelScaled(&Section, Localize("Laser"), 20.0f, TEXTALIGN_LEFT);
		MainView.VSplitLeft(5.0f, 0x0, &MainView);
		MainView.HSplitTop(5.0f, 0x0, &MainView);

		MainView.HSplitTop(50.0f, &Section, &MainView);
		Section.VSplitLeft(260.0f, &Section, 0x0);
		MainView.HSplitTop(25.0f, &SectionTwo, &MainView);

		static int LasterOutResetID, LaserInResetID;

		ColorHSLA LaserOutlineColor = DoLine_ColorPicker(&LasterOutResetID, 25.0f, 180.0f, 13.0f, 5.0f, &SectionTwo, Localize("Laser Outline Color"), &g_Config.m_ClLaserOutlineColor, ColorRGBA(0.074402f, 0.074402f, 0.247166f, 1.0f), false);

		MainView.HSplitTop(5.0f, 0x0, &MainView);
		MainView.HSplitTop(25.0f, &SectionTwo, &MainView);

		ColorHSLA LaserInnerColor = DoLine_ColorPicker(&LaserInResetID, 25.0f, 180.0f, 13.0f, 5.0f, &SectionTwo, Localize("Laser Inner Color"), &g_Config.m_ClLaserInnerColor, ColorRGBA(0.498039f, 0.498039f, 1.0f, 1.0f), false);

		Section.VSplitLeft(30.0f, 0, &Section);

		DoLaserPreview(&Section, LaserOutlineColor, LaserInnerColor);

		MainView.HSplitTop(25.0f, 0x0, &MainView);
		MainView.HSplitTop(20.0f, &SectionTwo, &MainView);

		UI()->DoLabelScaled(&SectionTwo, Localize("Hookline"), 20.0f, TEXTALIGN_LEFT);

		MainView.Margin(5.0f, &MainView);

		{
			CUIRect Button, Label;
			MainView.HSplitTop(5.0f, &Button, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(45.0f, &Label, &Button);
			UI()->DoLabelScaled(&Label, Localize("Size"), 14.0f, TEXTALIGN_LEFT);
			g_Config.m_ClHookCollSize = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClHookCollSize, &Button, g_Config.m_ClHookCollSize / 20.0f) * 20.0f);
		}

		{
			CUIRect Button, Label;
			MainView.HSplitTop(5.0f, &Button, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(45.0f, &Label, &Button);
			UI()->DoLabelScaled(&Label, Localize("Alpha"), 14.0f, TEXTALIGN_LEFT);
			g_Config.m_ClHookCollAlpha = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClHookCollAlpha, &Button, g_Config.m_ClHookCollAlpha / 100.0f) * 100.0f);
		}
		MainView.HSplitTop(5.0f, 0x0, &MainView);
		MainView.HSplitTop(25.0f, &SectionTwo, &MainView);

		static int HookCollNoCollResetID, HookCollHookableCollResetID, HookCollTeeCollResetID;
		DoLine_ColorPicker(&HookCollNoCollResetID, 25.0f, 180.0f, 13.0f, 5.0f, &SectionTwo, Localize("No hit"), &g_Config.m_ClHookCollColorNoColl, ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f), false);

		MainView.HSplitTop(5.0f, 0x0, &MainView);
		MainView.HSplitTop(25.0f, &SectionTwo, &MainView);

		DoLine_ColorPicker(&HookCollHookableCollResetID, 25.0f, 180.0f, 13.0f, 5.0f, &SectionTwo, Localize("Hookable"), &g_Config.m_ClHookCollColorHookableColl, ColorRGBA(130.0f / 255.0f, 232.0f / 255.0f, 160.0f / 255.0f, 1.0f), false);

		MainView.HSplitTop(5.0f, 0x0, &MainView);
		MainView.HSplitTop(25.0f, &SectionTwo, &MainView);

		DoLine_ColorPicker(&HookCollTeeCollResetID, 25.0f, 180.0f, 13.0f, 5.0f, &SectionTwo, Localize("Tee"), &g_Config.m_ClHookCollColorTeeColl, ColorRGBA(1.0f, 1.0f, 0.0f, 1.0f), false);
	}
	else if(s_CurTab == 1)
	{ // ***** CHAT TAB ***** //

		MainView.VSplitMid(&MainView, &Column);

		if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_ClChatOld, Localize("Use old chat style"), &g_Config.m_ClChatOld, &MainView, LineMargin))
			GameClient()->m_Chat.RebuildChat();

		MainView.HSplitTop(30.0f, 0x0, &MainView);

		// Message Colors and extra

		MainView.HSplitTop(20.0f, &Section, &MainView);
		MainView.HSplitTop(10.0f, 0x0, &MainView);

		UI()->DoLabelScaled(&Section, Localize("Messages"), 20.0f, TEXTALIGN_LEFT);

		const float LineSize = 25.0f;
		const float WantedPickerPosition = 210.0f;
		const float LabelSize = 13.0f;
		const float LineSpacing = 5.0f;

		char aBuf[64];

		int i = 0;
		static int ResetIDs[24];

		DoLine_ColorPicker(&ResetIDs[i++], LineSize, WantedPickerPosition, LabelSize, LineSpacing, &MainView, Localize("System message"), &g_Config.m_ClMessageSystemColor, ColorRGBA(1.0f, 1.0f, 0.5f), true, true, &g_Config.m_ClShowChatSystem);
		DoLine_ColorPicker(&ResetIDs[i++], LineSize, WantedPickerPosition, LabelSize, LineSpacing, &MainView, Localize("Highlighted message"), &g_Config.m_ClMessageHighlightColor, ColorRGBA(1.0f, 0.5f, 0.5f));
		DoLine_ColorPicker(&ResetIDs[i++], LineSize, WantedPickerPosition, LabelSize, LineSpacing, &MainView, Localize("Team message"), &g_Config.m_ClMessageTeamColor, ColorRGBA(0.65f, 1.0f, 0.65f));
		DoLine_ColorPicker(&ResetIDs[i++], LineSize, WantedPickerPosition, LabelSize, LineSpacing, &MainView, Localize("Friend message"), &g_Config.m_ClMessageFriendColor, ColorRGBA(1.0f, 0.137f, 0.137f), true, true, &g_Config.m_ClMessageFriend);
		DoLine_ColorPicker(&ResetIDs[i++], LineSize, WantedPickerPosition, LabelSize, LineSpacing, &MainView, Localize("Normal message"), &g_Config.m_ClMessageColor, ColorRGBA(1.0f, 1.0f, 1.0f));

		str_format(aBuf, sizeof(aBuf), "%s (echo)", Localize("Client message"));
		DoLine_ColorPicker(&ResetIDs[i++], LineSize, WantedPickerPosition, LabelSize, LineSpacing, &MainView, aBuf, &g_Config.m_ClMessageClientColor, ColorRGBA(0.5f, 0.78f, 1.0f));

		// ***** Chat Preview ***** //

		MainView = Column;

		MainView.HSplitTop(10.0f, 0x0, &MainView);
		MainView.HSplitTop(20.0f, &Section, &MainView);

		UI()->DoLabelScaled(&Section, Localize("Preview"), 20.0f, TEXTALIGN_LEFT);

		MainView.HSplitTop(10.0f, 0x0, &MainView);
		RenderTools()->DrawUIRect(&MainView, ColorRGBA(1, 1, 1, 0.1f), CUI::CORNER_ALL, 8.0f);
		MainView.HSplitTop(10.0f, 0x0, &MainView);

		ColorRGBA SystemColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageSystemColor));
		ColorRGBA HighlightedColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageHighlightColor));
		ColorRGBA TeamColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageTeamColor));
		ColorRGBA FriendColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageFriendColor));
		ColorRGBA NormalColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageColor));
		ColorRGBA ClientColor = color_cast<ColorRGBA, ColorHSLA>(ColorHSLA(g_Config.m_ClMessageClientColor));
		ColorRGBA DefaultNameColor(0.8f, 0.8f, 0.8f, 1.0f);

		constexpr float RealFontSize = CChat::FONT_SIZE * 2;
		const float RealMsgPaddingX = (!g_Config.m_ClChatOld ? CChat::MESSAGE_PADDING_X : 0) * 2;
		const float RealMsgPaddingY = (!g_Config.m_ClChatOld ? CChat::MESSAGE_PADDING_Y : 0) * 2;
		const float RealMsgPaddingTee = (!g_Config.m_ClChatOld ? CChat::MESSAGE_TEE_SIZE + CChat::MESSAGE_TEE_PADDING_RIGHT : 0) * 2;
		const float RealOffsetY = RealFontSize + RealMsgPaddingY;

		const float X = 5.0f + RealMsgPaddingX / 2.0f + MainView.x;
		float Y = MainView.y;

		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, X, Y, RealFontSize, TEXTFLAG_RENDER);

		str_copy(aBuf, Client()->PlayerName(), sizeof(aBuf));

		CAnimState *pIdleState = CAnimState::GetIdle();
		constexpr int PreviewTeeCount = 4;
		constexpr float RealTeeSize = CChat::MESSAGE_TEE_SIZE * 2;
		constexpr float RealTeeSizeHalved = CChat::MESSAGE_TEE_SIZE;
		constexpr float TWSkinUnreliableOffset = -0.25f;
		constexpr float OffsetTeeY = RealTeeSizeHalved;
		const float FullHeightMinusTee = RealOffsetY - RealTeeSize;

		CTeeRenderInfo RenderInfo[PreviewTeeCount];

		// Backgrounds first
		if(!g_Config.m_ClChatOld)
		{
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(0, 0, 0, 0.12f);

			char LineBuilder[128];
			float Width;
			float TempY = Y;
			constexpr float RealBackgroundRounding = CChat::MESSAGE_ROUNDING * 2.0f;

			if(g_Config.m_ClShowChatSystem)
			{
				str_format(LineBuilder, sizeof(LineBuilder), "*** '%s' entered and joined the game", aBuf);
				Width = TextRender()->TextWidth(0, RealFontSize, LineBuilder, -1, -1);
				RenderTools()->DrawRoundRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, CUI::CORNER_ALL);
				TempY += RealOffsetY;
			}

			str_format(LineBuilder, sizeof(LineBuilder), "%sRandom Tee: Hey, how are you %s?", g_Config.m_ClShowIDs ? " 7: " : "", aBuf);
			Width = TextRender()->TextWidth(0, RealFontSize, LineBuilder, -1, -1);
			RenderTools()->DrawRoundRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, CUI::CORNER_ALL);
			TempY += RealOffsetY;

			str_format(LineBuilder, sizeof(LineBuilder), "%sYour Teammate: Let's speedrun this!", g_Config.m_ClShowIDs ? "11: " : "");
			Width = TextRender()->TextWidth(0, RealFontSize, LineBuilder, -1, -1);
			RenderTools()->DrawRoundRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, CUI::CORNER_ALL);
			TempY += RealOffsetY;

			str_format(LineBuilder, sizeof(LineBuilder), "%s%sFriend: Hello there", g_Config.m_ClMessageFriend ? "♥ " : "", g_Config.m_ClShowIDs ? " 8: " : "");
			Width = TextRender()->TextWidth(0, RealFontSize, LineBuilder, -1, -1);
			RenderTools()->DrawRoundRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, CUI::CORNER_ALL);
			TempY += RealOffsetY;

			str_format(LineBuilder, sizeof(LineBuilder), "%sSpammer [6]: Hey fools, I'm spamming here!", g_Config.m_ClShowIDs ? " 9: " : "");
			Width = TextRender()->TextWidth(0, RealFontSize, LineBuilder, -1, -1);
			RenderTools()->DrawRoundRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX + RealMsgPaddingTee, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, CUI::CORNER_ALL);
			TempY += RealOffsetY;

			Width = TextRender()->TextWidth(0, RealFontSize, "*** Echo command executed", -1, -1);
			RenderTools()->DrawRoundRectExt(X - RealMsgPaddingX / 2.0f, TempY - RealMsgPaddingY / 2.0f, Width + RealMsgPaddingX, RealFontSize + RealMsgPaddingY, RealBackgroundRounding, CUI::CORNER_ALL);

			Graphics()->QuadsEnd();

			// Load skins

			int DefaultInd = GameClient()->m_Skins.Find("default");

			for(auto &Info : RenderInfo)
			{
				Info.m_Size = RealTeeSize;
				Info.m_CustomColoredSkin = false;
			}

			int ind = -1;
			int pos = 0;

			RenderInfo[pos++].m_OriginalRenderSkin = GameClient()->m_Skins.Get(DefaultInd)->m_OriginalSkin;
			RenderInfo[pos++].m_OriginalRenderSkin = (ind = GameClient()->m_Skins.Find("pinky")) != -1 ? GameClient()->m_Skins.Get(ind)->m_OriginalSkin : RenderInfo[0].m_OriginalRenderSkin;
			RenderInfo[pos++].m_OriginalRenderSkin = (ind = GameClient()->m_Skins.Find("cammostripes")) != -1 ? GameClient()->m_Skins.Get(ind)->m_OriginalSkin : RenderInfo[0].m_OriginalRenderSkin;
			RenderInfo[pos++].m_OriginalRenderSkin = (ind = GameClient()->m_Skins.Find("beast")) != -1 ? GameClient()->m_Skins.Get(ind)->m_OriginalSkin : RenderInfo[0].m_OriginalRenderSkin;
		}

		// System
		if(g_Config.m_ClShowChatSystem)
		{
			TextRender()->TextColor(SystemColor);
			TextRender()->TextEx(&Cursor, "*** '", -1);
			TextRender()->TextEx(&Cursor, aBuf, -1);
			TextRender()->TextEx(&Cursor, "' entered and joined the game", -1);
			TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);
		}

		// Highlighted
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		TextRender()->TextColor(DefaultNameColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, " 7: ", -1);
		TextRender()->TextEx(&Cursor, "Random Tee: ", -1);
		TextRender()->TextColor(HighlightedColor);
		TextRender()->TextEx(&Cursor, "Hey, how are you ", -1);
		TextRender()->TextEx(&Cursor, aBuf, -1);
		TextRender()->TextEx(&Cursor, "?", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &RenderInfo[1], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Team
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		TextRender()->TextColor(TeamColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, "11: ", -1);
		TextRender()->TextEx(&Cursor, "Your Teammate: Let's speedrun this!", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &RenderInfo[0], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Friend
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		if(g_Config.m_ClMessageFriend)
		{
			TextRender()->TextColor(FriendColor);
			TextRender()->TextEx(&Cursor, "♥ ", -1);
		}
		TextRender()->TextColor(DefaultNameColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, " 8: ", -1);
		TextRender()->TextEx(&Cursor, "Friend: ", -1);
		TextRender()->TextColor(NormalColor);
		TextRender()->TextEx(&Cursor, "Hello there", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &RenderInfo[2], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Normal
		TextRender()->MoveCursor(&Cursor, RealMsgPaddingTee, 0);
		TextRender()->TextColor(DefaultNameColor);
		if(g_Config.m_ClShowIDs)
			TextRender()->TextEx(&Cursor, " 9: ", -1);
		TextRender()->TextEx(&Cursor, "Spammer ", -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.3f);
		TextRender()->TextEx(&Cursor, "[6]", -1);
		TextRender()->TextColor(NormalColor);
		TextRender()->TextEx(&Cursor, ": Hey fools, I'm spamming here!", -1);
		if(!g_Config.m_ClChatOld)
			RenderTools()->RenderTee(pIdleState, &RenderInfo[3], EMOTE_NORMAL, vec2(1, 0.1f), vec2(X + RealTeeSizeHalved, Y + OffsetTeeY + FullHeightMinusTee / 2.0f + TWSkinUnreliableOffset));
		TextRender()->SetCursorPosition(&Cursor, X, Y += RealOffsetY);

		// Client
		TextRender()->TextColor(ClientColor);
		TextRender()->TextEx(&Cursor, "*** Echo command executed", -1);
		TextRender()->SetCursorPosition(&Cursor, X, Y);

		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}
}

void CMenus::RenderSettingsDDNet(CUIRect MainView)
{
	CUIRect Button, Left, Right, LeftLeft, Demo, Gameplay, Miscellaneous, Label, Background;

	bool CheckSettings = false;
	static int s_InpMouseOld = g_Config.m_InpMouseOld;

	MainView.HSplitTop(100.0f, &Demo, &MainView);

	Demo.HSplitTop(30.0f, &Label, &Demo);
	UI()->DoLabelScaled(&Label, Localize("Demo"), 20.0f, TEXTALIGN_LEFT);
	Demo.Margin(5.0f, &Demo);
	Demo.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClAutoRaceRecord, Localize("Save the best demo of each race"), g_Config.m_ClAutoRaceRecord, &Button))
	{
		g_Config.m_ClAutoRaceRecord ^= 1;
	}

	{
		Left.HSplitTop(20.0f, &Button, &Left);

		if(DoButton_CheckBox(&g_Config.m_ClReplays, Localize("Enable replays"), g_Config.m_ClReplays, &Button))
		{
			g_Config.m_ClReplays ^= 1;
			if(!g_Config.m_ClReplays)
			{
				// stop recording and remove the tmp demo file
				Client()->DemoRecorder_Stop(RECORDER_REPLAYS, true);
			}
			else
			{
				// start recording
				Client()->DemoRecorder_HandleAutoStart();
			}
		}

		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(140.0f, &Label, &Button);
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), Localize("Default length: %d"), g_Config.m_ClReplayLength);
		UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);

		int NewValue = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClReplayLength, &Button, (minimum(g_Config.m_ClReplayLength, 600) - 10) / 590.0f) * 590.0f) + 10;
		if(g_Config.m_ClReplayLength < 600 || NewValue < 600)
			g_Config.m_ClReplayLength = minimum(NewValue, 600);
	}

	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ClRaceGhost, Localize("Ghost"), g_Config.m_ClRaceGhost, &Button))
	{
		g_Config.m_ClRaceGhost ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClRaceGhost, &Button, Localize("When you cross the start line, show a ghost tee replicating the movements of your best time"));

	if(g_Config.m_ClRaceGhost)
	{
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClRaceShowGhost, Localize("Show ghost"), g_Config.m_ClRaceShowGhost, &Button))
		{
			g_Config.m_ClRaceShowGhost ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClRaceSaveGhost, Localize("Save ghost"), g_Config.m_ClRaceSaveGhost, &Button))
		{
			g_Config.m_ClRaceSaveGhost ^= 1;
		}
	}

	MainView.HSplitTop(330.0f, &Gameplay, &MainView);

	Gameplay.HSplitTop(30.0f, &Label, &Gameplay);
	UI()->DoLabelScaled(&Label, Localize("Gameplay"), 20.0f, TEXTALIGN_LEFT);
	Gameplay.Margin(5.0f, &Gameplay);
	Gameplay.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	{
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(120.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Overlay entities"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_ClOverlayEntities = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClOverlayEntities, &Button, g_Config.m_ClOverlayEntities / 100.0f) * 100.0f);
	}

	{
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitMid(&LeftLeft, &Button);

		Button.VSplitLeft(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Size"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_ClTextEntitiesSize = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClTextEntitiesSize, &Button, g_Config.m_ClTextEntitiesSize / 100.0f) * 100.0f);

		if(DoButton_CheckBox(&g_Config.m_ClTextEntities, Localize("Show text entities"), g_Config.m_ClTextEntities, &LeftLeft))
		{
			g_Config.m_ClTextEntities ^= 1;
		}
	}

	{
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitMid(&LeftLeft, &Button);

		Button.VSplitLeft(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Opacity"), 14.0f, TEXTALIGN_LEFT);
		g_Config.m_ClShowOthersAlpha = (int)(UIEx()->DoScrollbarH(&g_Config.m_ClShowOthersAlpha, &Button, g_Config.m_ClShowOthersAlpha / 100.0f) * 100.0f);

		if(DoButton_CheckBox(&g_Config.m_ClShowOthers, Localize("Show others"), g_Config.m_ClShowOthers == SHOW_OTHERS_ON, &LeftLeft))
		{
			g_Config.m_ClShowOthers = g_Config.m_ClShowOthers != SHOW_OTHERS_ON ? SHOW_OTHERS_ON : SHOW_OTHERS_OFF;
		}

		GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClShowOthersAlpha, &Button, "Adjust the opacity of entities belonging to other teams, such as tees and nameplates");
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	static int s_ShowOwnTeamID = 0;
	if(DoButton_CheckBox(&s_ShowOwnTeamID, Localize("Show others (own team only)"), g_Config.m_ClShowOthers == SHOW_OTHERS_ONLY_TEAM, &Button))
	{
		g_Config.m_ClShowOthers = g_Config.m_ClShowOthers != SHOW_OTHERS_ONLY_TEAM ? SHOW_OTHERS_ONLY_TEAM : SHOW_OTHERS_OFF;
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClShowQuads, Localize("Show quads"), g_Config.m_ClShowQuads, &Button))
	{
		g_Config.m_ClShowQuads ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClShowQuads, &Button, Localize("Quads are used for background decoration"));

	Right.HSplitTop(20.0f, &Label, &Right);
	Label.VSplitLeft(130.0f, &Label, &Button);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Default zoom"), g_Config.m_ClDefaultZoom);
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);
	g_Config.m_ClDefaultZoom = static_cast<int>(UIEx()->DoScrollbarH(&g_Config.m_ClDefaultZoom, &Button, g_Config.m_ClDefaultZoom / 20.0f) * 20.0f + 0.1f);

	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ClAntiPing, Localize("AntiPing"), g_Config.m_ClAntiPing, &Button))
	{
		g_Config.m_ClAntiPing ^= 1;
	}
	GameClient()->m_Tooltips.DoToolTip(&g_Config.m_ClAntiPing, &Button, Localize("Tries to predict other entities to give a feel of low latency"));

	if(g_Config.m_ClAntiPing)
	{
		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClAntiPingPlayers, Localize("AntiPing: predict other players"), g_Config.m_ClAntiPingPlayers, &Button))
		{
			g_Config.m_ClAntiPingPlayers ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClAntiPingWeapons, Localize("AntiPing: predict weapons"), g_Config.m_ClAntiPingWeapons, &Button))
		{
			g_Config.m_ClAntiPingWeapons ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		if(DoButton_CheckBox(&g_Config.m_ClAntiPingGrenade, Localize("AntiPing: predict grenade paths"), g_Config.m_ClAntiPingGrenade, &Button))
		{
			g_Config.m_ClAntiPingGrenade ^= 1;
		}
	}
	else
	{
		Right.HSplitTop(60.0f, 0, &Right);
	}

	Right.HSplitTop(40.0f, 0, &Right);

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClShowHookCollOther, Localize("Show other players' hook collision lines"), g_Config.m_ClShowHookCollOther, &Button))
	{
		g_Config.m_ClShowHookCollOther = g_Config.m_ClShowHookCollOther >= 1 ? 0 : 1;
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClShowDirection, Localize("Show other players' key presses"), g_Config.m_ClShowDirection >= 1, &Button))
	{
		g_Config.m_ClShowDirection = g_Config.m_ClShowDirection >= 1 ? 0 : 1;
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	static int s_ShowLocalPlayer = 0;
	if(DoButton_CheckBox(&s_ShowLocalPlayer, Localize("Show local player's key presses"), g_Config.m_ClShowDirection == 2, &Button))
	{
		g_Config.m_ClShowDirection = g_Config.m_ClShowDirection != 2 ? 2 : 1;
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_InpMouseOld, Localize("Old mouse mode"), g_Config.m_InpMouseOld, &Button))
	{
		g_Config.m_InpMouseOld ^= 1;
		CheckSettings = true;
	}

	if(CheckSettings)
		m_NeedRestartDDNet = s_InpMouseOld != g_Config.m_InpMouseOld;

	Left.HSplitTop(5.0f, &Button, &Left);
	Right.HSplitTop(25.0f, &Button, &Right);
	Left.VSplitRight(10.0f, &Left, 0x0);
	Right.VSplitLeft(10.0f, 0x0, &Right);
	Left.HSplitTop(25.0f, 0x0, &Left);
	CUIRect TempLabel;
	Left.HSplitTop(25.0f, &TempLabel, &Left);
	Left.HSplitTop(5.0f, 0x0, &Left);

	UI()->DoLabelScaled(&TempLabel, Localize("Background"), 20.0f, TEXTALIGN_LEFT);

	Right.HSplitTop(45.0f, 0x0, &Right);
	Right.HSplitTop(25.0f, &TempLabel, &Right);
	Right.HSplitTop(5.0f, 0x0, &Miscellaneous);

	UI()->DoLabelScaled(&TempLabel, Localize("Miscellaneous"), 20.0f, TEXTALIGN_LEFT);

	static int ResetID1 = 0;
	static int ResetID2 = 0;
	ColorRGBA GreyDefault(0.5f, 0.5f, 0.5f, 1);
	DoLine_ColorPicker(&ResetID2, 25.0f, 194.0f, 13.0f, 5.0f, &Left, Localize("Entities Background color"), &g_Config.m_ClBackgroundEntitiesColor, GreyDefault, false);

	Left.VSplitLeft(5.0f, 0x0, &Left);
	Left.HSplitTop(25.0f, &Background, &Left);
	Background.HSplitTop(20.0f, &Background, 0);
	Background.VSplitLeft(100.0f, &Label, &TempLabel);
	UI()->DoLabelScaled(&Label, Localize("Map"), 14.0f, TEXTALIGN_LEFT);
	static float s_Map = 0.0f;
	UIEx()->DoEditBox(g_Config.m_ClBackgroundEntities, &TempLabel, g_Config.m_ClBackgroundEntities, sizeof(g_Config.m_ClBackgroundEntities), 14.0f, &s_Map);

	Left.HSplitTop(20.0f, &Button, &Left);
	bool UseCurrentMap = str_comp(g_Config.m_ClBackgroundEntities, CURRENT_MAP) == 0;
	static int s_UseCurrentMapID = 0;
	if(DoButton_CheckBox(&s_UseCurrentMapID, Localize("Use current map as background"), UseCurrentMap, &Button))
	{
		if(UseCurrentMap)
			g_Config.m_ClBackgroundEntities[0] = '\0';
		else
			str_copy(g_Config.m_ClBackgroundEntities, CURRENT_MAP, sizeof(g_Config.m_ClBackgroundEntities));
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&g_Config.m_ClBackgroundShowTilesLayers, Localize("Show tiles layers from BG map"), g_Config.m_ClBackgroundShowTilesLayers, &Button))
		g_Config.m_ClBackgroundShowTilesLayers ^= 1;

	Miscellaneous.HSplitTop(25.0f, &Button, &Right);
	DoLine_ColorPicker(&ResetID1, 25.0f, 194.0f, 13.0f, 5.0f, &Button, Localize("Regular Background Color"), &g_Config.m_ClBackgroundColor, GreyDefault, false);
	Right.HSplitTop(5.0f, 0x0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&g_Config.m_ClHttpMapDownload, Localize("Try fast HTTP map download first"), g_Config.m_ClHttpMapDownload, &Button))
		g_Config.m_ClHttpMapDownload ^= 1;

	static int s_ButtonTimeout = 0;
	Right.HSplitTop(10.0f, 0x0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_Menu(&s_ButtonTimeout, Localize("New random timeout code"), 0, &Button))
	{
		Client()->GenerateTimeoutSeed();
	}

	static float s_RunOnJoin = 0.0f;
	Right.HSplitTop(5.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Label, &Right);
	Label.VSplitLeft(5.0f, 0, &Label);
	UI()->DoLabelScaled(&Label, Localize("Run on join"), 14.0f, TEXTALIGN_LEFT);
	Right.HSplitTop(20.0f, &Button, &Right);
	Button.VSplitLeft(5.0f, 0, &Button);
	SUIExEditBoxProperties EditProps;
	EditProps.m_pEmptyText = Localize("Chat command (e.g. showall 1)");
	UIEx()->DoEditBox(g_Config.m_ClRunOnJoin, &Button, g_Config.m_ClRunOnJoin, sizeof(g_Config.m_ClRunOnJoin), 14.0f, &s_RunOnJoin, false, CUI::CORNER_ALL, EditProps);
	// Updater
#if defined(CONF_AUTOUPDATE)
	{
		MainView.VSplitMid(&Left, &Right);
		Left.w += 20.0f;
		Left.HSplitBottom(20.0f, 0x0, &Label);
		bool NeedUpdate = str_comp(Client()->LatestVersion(), "0");
		int State = Updater()->GetCurrentState();

		// Update Button
		if(NeedUpdate && State <= IUpdater::CLEAN)
		{
			str_format(aBuf, sizeof(aBuf), Localize("DDNet %s is available:"), Client()->LatestVersion());
			Label.VSplitLeft(TextRender()->TextWidth(0, 14.0f, aBuf, -1, -1.0f) + 10.0f, &Label, &Button);
			Button.VSplitLeft(100.0f, &Button, 0);
			static int s_ButtonUpdate = 0;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Update now"), 0, &Button))
			{
				Updater()->InitiateUpdate();
			}
		}
		else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
			str_format(aBuf, sizeof(aBuf), Localize("Updating..."));
		else if(State == IUpdater::NEED_RESTART)
		{
			str_format(aBuf, sizeof(aBuf), Localize("DDNet Client updated!"));
			m_NeedRestartUpdate = true;
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), Localize("No updates available"));
			Label.VSplitLeft(TextRender()->TextWidth(0, 14.0f, aBuf, -1, -1.0f) + 10.0f, &Label, &Button);
			Button.VSplitLeft(100.0f, &Button, 0);
			static int s_ButtonUpdate = 0;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Check now"), 0, &Button))
			{
				Client()->RequestDDNetInfo();
			}
		}
		UI()->DoLabelScaled(&Label, aBuf, 14.0f, TEXTALIGN_LEFT);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
#endif
}
