// TWLoader: Settings screen.
#include "settings.h"
#include "date.h"
#include "sound.h"
#include "download.h"
#include "log.h"

#include <unistd.h>
#include <string>
using std::string;

#include <3ds.h>
#include <sf2d.h>
#include <sfil.h>
#include <sftd.h>

// Functions from main.cpp.
const char* text_returntohomemenu(void);
void draw_volume_slider(sf2d_texture *texarray[]);

extern sf2d_texture *batteryIcon;	// Current battery level icon.
void update_battery_level(sf2d_texture *texchrg, sf2d_texture *texarray[]);

// Variables from main.cpp.
extern sf2d_texture *shoulderLtex;
extern sf2d_texture *shoulderRtex;
extern const char* Lshouldertext;
extern const char* Rshouldertext;
extern int LshoulderYpos;
extern int RshoulderYpos;

extern sftd_font *font;
extern sftd_font *font_b;

extern int fadealpha;
extern bool fadein;
extern bool fadeout;

extern sf2d_texture *settingslogotex;	// TWLoader logo.
extern char settings_vertext[13];

extern string name;

// Sound effects from main.cpp.
extern sound *sfx_select;
extern sound *sfx_switch;
extern sound *sfx_back;

extern int titleboxXmovetimer; // Set to 1 for fade-in effect to run

// Textures.

/** Top screen **/
extern bool dspfirmfound;
static bool settings_tex_loaded = false;
static sf2d_texture *whomeicontex = NULL;	// HOME icon.
static sf2d_texture *setvoltex[5] = { };	// Volume levels.
static sf2d_texture *setbatterychrgtex = NULL;	// Fully charged.
static sf2d_texture *setbatterytex[6] = { };	// Battery levels.

static sf2d_texture *dsboottex = NULL;		// DS boot screen
static sf2d_texture *dsiboottex = NULL;	// DSi boot screen
static sf2d_texture *dshstex = NULL;		// DS H&S screen
static sf2d_texture *dsihstex = NULL;		// DSi H&S screen
static sf2d_texture *disabledtex = NULL;	// Red circle with line

/** Bottom screen **/
sf2d_texture *settingstex = NULL;

enum SubScreenMode {
	SUBSCREEN_MODE_FRONTEND = 0,	// Frontend settings
	SUBSCREEN_MODE_NTR_TWL = 1,	// NTR/TWL-mode settings
	SUBSCREEN_MODE_FLASH_CARD = 2,	// Flash card options
};
static SubScreenMode subscreenmode = SUBSCREEN_MODE_FRONTEND;

/** Settings **/

// Color settings.
// Use SET_ALPHA() to replace the alpha value.
const ColorData *color_data = NULL;
u32 menucolor;

// 3D offsets. (0 == Left, 1 == Right)
Offset3D offset3D[2] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

// Cursor position. (one per subscreen)
static int cursor_pos[3] = {0, 0, 0};

// Location of the bottom screen image.
const char* bottomloc = NULL;

int settings_colorvalue;
int settings_menucolorvalue;
int settings_filenamevalue;
int settings_locswitchvalue;
int settings_topbordervalue;
int settings_countervalue;
int settings_custombotvalue;
int settings_autoupdatevalue;
int settings_autodlvalue;

int twlsettings_rainbowledvalue;
int twlsettings_cpuspeedvalue;
int twlsettings_extvramvalue;
int twlsettings_forwardervalue;
int twlsettings_flashcardvalue;
/* Flashcard value
	0: DSTT/R4i Gold/R4i-SDHC/R4 SDHC Dual-Core/R4 SDHC Upgrade/SC DSONE
	1: R4DS (Original Non-SDHC version)/ M3 Simply
	2: R4iDSN/R4i Gold RTS
	3: Acekard 2(i)/M3DS Real/R4i-SDHC v1.4.x
	4: Acekard RPG
	5: Ace 3DS+/Gateway Blue Card/R4iTT
	6: SuperCard DSTWO
*/
int twlsettings_bootscreenvalue;
int twlsettings_healthsafetyvalue;
int twlsettings_launchslot1value;
int twlsettings_resetslot1value;
int twlsettings_consolevalue;
int twlsettings_lockarm9scfgextvalue;

/** Strings **/

// Frontend
static const char settings_xbuttontext[] = "X: Update bootstrap (Official Release)";
static const char settings_ybuttontext[] = "Y: Update bootstrap (Unofficial build)";
static const char settings_startbuttontext[] = "START: Update TWLoader";

static const char settings_colortext[] = "Color";
static const char settings_menucolortext[] = "Menu color";
static const char settings_filenametext[] = "Show filename";
static const char settings_locswitchtext[] = "Game location switcher";
static const char settings_topbordertext[] = "Top border";
static const char settings_countertext[] = "Game counter";
static const char settings_custombottext[] = "Custom bottom image";
static const char settings_autoupdatetext[] = "Auto-update bootstrap";
static const char settings_autodltext[] = "Auto-update to latest TWLoader";

static const char settings_lrpicktext[] = "Left/Right: Pick";
static const char settings_absavereturn[] = "A/B: Save and Return";

// NTR/TWL-mode Settings text
static const char twlsettings_flashcardtext[] = "Flashcard(s) select";
static const char twlsettings_rainbowledtext[] = "Rainbow LED";
static const char twlsettings_cpuspeedtext[] = "ARM9 CPU Speed";
static const char twlsettings_extvramtext[] = "VRAM boost";
static const char twlsettings_bootscreentext[] = "DS/DSi Boot Screen";
static const char twlsettings_healthsafetytext[] = "Health and Safety message";
static const char twlsettings_resetslot1text[] = "Reset Slot-1";
static const char twlsettings_consoletext[] = "Console output";
static const char twlsettings_lockarm9scfgexttext[] = "Lock ARM9 SCFG_EXT";

/**
 * Reset the settings screen's subscreen mode.
 */
void settingsResetSubScreenMode(void)
{
	subscreenmode = SUBSCREEN_MODE_FRONTEND;
	memset(cursor_pos, 0, sizeof(cursor_pos));
}

/**
 * Load the settings textures.
 */
void settingsLoadTextures(void)
{
	if (settings_tex_loaded)
		return;

	/** Top screen **/
	setvoltex[0] = sfil_load_PNG_file("romfs:/graphics/settings/volume0.png", SF2D_PLACE_RAM); // Show no volume (settings)
	setvoltex[1] = sfil_load_PNG_file("romfs:/graphics/settings/volume1.png", SF2D_PLACE_RAM); // Volume low above 0 (settings)
	setvoltex[2] = sfil_load_PNG_file("romfs:/graphics/settings/volume2.png", SF2D_PLACE_RAM); // Volume medium (settings)
	setvoltex[3] = sfil_load_PNG_file("romfs:/graphics/settings/volume3.png", SF2D_PLACE_RAM); // Hight volume (settings)
	setvoltex[4] = sfil_load_PNG_file("romfs:/graphics/settings/volume4.png", SF2D_PLACE_RAM); // No DSP firm found (settings)

	setbatterychrgtex = sfil_load_PNG_file("romfs:/graphics/settings/battery_charging.png", SF2D_PLACE_RAM);
	setbatterytex[0] = sfil_load_PNG_file("romfs:/graphics/settings/battery0.png", SF2D_PLACE_RAM);
	setbatterytex[1] = sfil_load_PNG_file("romfs:/graphics/settings/battery1.png", SF2D_PLACE_RAM);
	setbatterytex[2] = sfil_load_PNG_file("romfs:/graphics/settings/battery2.png", SF2D_PLACE_RAM);
	setbatterytex[3] = sfil_load_PNG_file("romfs:/graphics/settings/battery3.png", SF2D_PLACE_RAM);
	setbatterytex[4] = sfil_load_PNG_file("romfs:/graphics/settings/battery4.png", SF2D_PLACE_RAM);
	setbatterytex[5] = sfil_load_PNG_file("romfs:/graphics/settings/battery5.png", SF2D_PLACE_RAM);

	dsboottex = sfil_load_PNG_file("romfs:/graphics/settings/dsboot.png", SF2D_PLACE_RAM); // DS boot screen in settings
	dsiboottex = sfil_load_PNG_file("romfs:/graphics/settings/dsiboot.png", SF2D_PLACE_RAM); // DSi boot screen in settings
	dshstex = sfil_load_PNG_file("romfs:/graphics/settings/dshs.png", SF2D_PLACE_RAM); // DS H&S screen in settings
	dsihstex = sfil_load_PNG_file("romfs:/graphics/settings/dsihs.png", SF2D_PLACE_RAM); // DSi H&S screen in settings
	disabledtex = sfil_load_PNG_file("romfs:/graphics/settings/disable.png", SF2D_PLACE_RAM); // Red circle with line

	/** Bottom screen **/
	settingstex = sfil_load_PNG_file("romfs:/graphics/settings/screen.png", SF2D_PLACE_RAM); // Bottom of settings screen
	whomeicontex = sfil_load_PNG_file("romfs:/graphics/settings/whomeicon.png", SF2D_PLACE_RAM); // HOME icon

	// All textures loaded.
	settings_tex_loaded = true;
}

/**
 * Unload the settings textures.
 */
void settingsUnloadTextures(void)
{
	if (!settings_tex_loaded)
		return;

	/** Top screen **/
	for (int i = 0; i < 5; i++) {
		sf2d_free_texture(setvoltex[i]);
		setvoltex[i] = NULL;
	}
	sf2d_free_texture(setbatterychrgtex);

	for (int i = 0; i < 6; i++) {
		sf2d_free_texture(setbatterytex[i]);
		setbatterytex[i] = NULL;
	}

	sf2d_free_texture(dsboottex);
	dsboottex = NULL;
	sf2d_free_texture(dsiboottex);
	dsiboottex = NULL;
	sf2d_free_texture(dshstex);
	dshstex = NULL;
	sf2d_free_texture(dsihstex);
	dsihstex = NULL;
	sf2d_free_texture(disabledtex);
	disabledtex = NULL;

	/** Bottom screen **/
	sf2d_free_texture(settingstex);
	settingstex = NULL;
	sf2d_free_texture(whomeicontex);
	whomeicontex = NULL;

	// All textures unloaded.
	settings_tex_loaded = false;
}

/**
 * Draw the top settings screen.
 */
void settingsDrawTopScreen(void)
{
	/* if (!musicbool) {
		if (dspfirmfound) { bgm_settings->play(); }
		musicbool = true;
	} */
	if (!settings_tex_loaded) {
		settingsLoadTextures();
	}

	update_battery_level(setbatterychrgtex, setbatterytex);
	sf2d_start_frame(GFX_TOP, GFX_LEFT);
	sf2d_draw_texture_scale(settingstex, 0, 0, 1.32, 1);
	if (subscreenmode == SUBSCREEN_MODE_NTR_TWL) {
		if (twlsettings_cpuspeedvalue == 1) {
			sf2d_draw_texture(dsiboottex, offset3D[0].boxart+136, 20); // Draw boot screen
		} else {
			sf2d_draw_texture(dsboottex, offset3D[0].boxart+136, 20); // Draw boot screen
		}
		if (twlsettings_healthsafetyvalue == 1) {
			if (twlsettings_cpuspeedvalue == 1) {
				sf2d_draw_texture(dsihstex, offset3D[0].boxart+136, 124); // Draw H&S screen
			} else {
				sf2d_draw_texture(dshstex, offset3D[0].boxart+136, 124); // Draw H&S screen
			}
		} else {
			// Draw a white screen in place of the H&S screen.
			sf2d_draw_rectangle(offset3D[0].boxart+136, 124, 128, 96, RGBA8(255, 255, 255, 255));
		}
		if (twlsettings_bootscreenvalue == 0) {
			sf2d_draw_texture(disabledtex, offset3D[0].disabled+136, 20); // Draw disabled texture
			sf2d_draw_texture(disabledtex, offset3D[0].disabled+136, 124); // Draw disabled texture
		}
	} else {
		sf2d_draw_texture(settingslogotex, offset3D[0].boxart+400/2 - settingslogotex->width/2, 240/2 - settingslogotex->height/2);
		if (subscreenmode == SUBSCREEN_MODE_FRONTEND) {
			sftd_draw_textf(font, offset3D[0].disabled+72, 166, RGBA8(0, 0, 255, 255), 14, settings_xbuttontext);
			sftd_draw_textf(font, offset3D[0].disabled+72, 180, RGBA8(0, 255, 0, 255), 14, settings_ybuttontext);
			sftd_draw_textf(font, offset3D[0].disabled+72, 194, RGBA8(255, 255, 255, 255), 14, settings_startbuttontext);
		}
	}
	sftd_draw_text(font, 328, 3, RGBA8(255, 255, 255, 255), 12, RetTime().c_str());
	sftd_draw_textf(font, 334, 222, RGBA8(255, 255, 255, 255), 14, settings_vertext);

	draw_volume_slider(setvoltex);
	sf2d_draw_texture(batteryIcon, 371, 2);
	sftd_draw_textf(font, 32, 2, SET_ALPHA(color_data->color, 255), 12, name.c_str());
	sf2d_draw_rectangle(0, 0, 400, 240, RGBA8(0, 0, 0, fadealpha)); // Fade in/out effect
	sf2d_end_frame();

	sf2d_start_frame(GFX_TOP, GFX_RIGHT);
	sf2d_draw_texture_scale(settingstex, 0, 0, 1.32, 1);
	if (subscreenmode == SUBSCREEN_MODE_NTR_TWL) {
		if (twlsettings_cpuspeedvalue == 1) {
			sf2d_draw_texture(dsiboottex, offset3D[1].boxart+136, 20); // Draw boot screen
		} else {
			sf2d_draw_texture(dsboottex, offset3D[1].boxart+136, 20); // Draw boot screen
		}
		if (twlsettings_healthsafetyvalue == 1) {
			if (twlsettings_cpuspeedvalue == 1) {
				sf2d_draw_texture(dsihstex, offset3D[1].boxart+136, 124); // Draw H&S screen
			} else {
				sf2d_draw_texture(dshstex, offset3D[1].boxart+136, 124); // Draw H&S screen
			}
		} else {
			// Draw a white screen in place of the H&S screen.
			sf2d_draw_rectangle(offset3D[1].boxart+136, 124, 128, 96, RGBA8(255, 255, 255, 255));
		}
		if (twlsettings_bootscreenvalue == 0) {
			sf2d_draw_texture(disabledtex, offset3D[1].disabled+136, 20); // Draw disabled texture
			sf2d_draw_texture(disabledtex, offset3D[1].disabled+136, 124); // Draw disabled texture
		}
	} else {
		sf2d_draw_texture(settingslogotex, offset3D[1].boxart+400/2 - settingslogotex->width/2, 240/2 - settingslogotex->height/2);
		if (subscreenmode == SUBSCREEN_MODE_FRONTEND) {
			sftd_draw_textf(font, offset3D[1].disabled+72, 166, RGBA8(0, 0, 255, 255), 14, settings_xbuttontext);
			sftd_draw_textf(font, offset3D[1].disabled+72, 180, RGBA8(0, 255, 0, 255), 14, settings_ybuttontext);
			sftd_draw_textf(font, offset3D[1].disabled+72, 194, RGBA8(255, 255, 255, 255), 14, settings_startbuttontext);
		}
	}
	sftd_draw_text(font, 328, 3, RGBA8(255, 255, 255, 255), 12, RetTime().c_str());
	sftd_draw_textf(font, 334, 222, RGBA8(255, 255, 255, 255), 14, settings_vertext);
	draw_volume_slider(setvoltex);
	sf2d_draw_texture(batteryIcon, 371, 2);
	sftd_draw_textf(font, 32, 2, SET_ALPHA(color_data->color, 255), 12, name.c_str());
	sf2d_draw_rectangle(0, 0, 400, 240, RGBA8(0, 0, 0, fadealpha)); // Fade in/out effect
	sf2d_end_frame();
}

/**
 * Draw the bottom settings screen.
 */
void settingsDrawBottomScreen(void)
{
	if (!settings_tex_loaded) {
		settingsLoadTextures();
	}

	sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
	sf2d_draw_texture(settingstex, 0, 0);
	sf2d_draw_texture(whomeicontex, 81, 220); // Draw HOME icon
	sftd_draw_textf(font, 98, 221, RGBA8(255, 255, 255, 255), 13, text_returntohomemenu());

	// X positions.
	static const int Xpos = 24;
	static const int XposValue = 236;
	// Title for the bottom screen.
	const char *title = "";

	if (subscreenmode == SUBSCREEN_MODE_FRONTEND) {
		sf2d_draw_texture(shoulderLtex, 0, LshoulderYpos);
		sf2d_draw_texture(shoulderRtex, 248, RshoulderYpos);
		sftd_draw_textf(font, 17, LshoulderYpos+5, RGBA8(0, 0, 0, 255), 11, Lshouldertext);
		sftd_draw_textf(font, 252, RshoulderYpos+5, RGBA8(0, 0, 0, 255), 11, Rshouldertext);

		// Color text.
		static const char *const color_text[] = {
			"Gray", "Brown", "Red", "Pink",
			"Orange", "Yellow", "Yellow-Green", "Green 1",
			"Green 2", "Light Green", "Sky Blue", "Light Blue",
			"Blue", "Violet", "Purple", "Fuchsia",
			"Red & Blue", "Green & Yellow", "Christmas"
		};
		if (settings_colorvalue < 0 || settings_colorvalue > 18)
			settings_colorvalue = 0;
		const char *const colorvaluetext = color_text[settings_colorvalue];

		// Menu color text.
		static const char *const menu_color_text[] = {
			"White", "Black", "Brown", "Red",
			"Pink", "Orange", "Yellow", "Yellow-Green",
			"Green 1", "Green 2", "Light Green", "Sky Blue",
			"Light Blue", "Blue", "Violet", "Purple",
			"Fuchsia"
		};
		if (settings_menucolorvalue < 0 || settings_menucolorvalue > 18)
			settings_menucolorvalue = 0;
		const char *const menucolorvaluetext = menu_color_text[settings_menucolorvalue];

		const char *const filenamevaluetext = (settings_filenamevalue ? "On" : "Off");
		const char *const locswitchvaluetext = (settings_locswitchvalue ? "On" : "Off");
		const char *const topbordervaluetext = (settings_topbordervalue ? "On" : "Off");
		const char *const countervaluetext = (settings_countervalue ? "On" : "Off");
		const char *const custombotvaluetext = (settings_custombotvalue ? "On" : "Off");

		const char *autoupdatevaluetext;
		switch (settings_autoupdatevalue) {
			case 0:
			default:
				autoupdatevaluetext = "Off";
				break;
			case 1:
				autoupdatevaluetext = "Release";
				break;
			case 2:
				autoupdatevaluetext = "Unofficial";
				break;
		}
		const char *autodlvaluetext = (settings_autodlvalue ? "On" : "Off");

		title = "Settings: GUI";
		int Ypos = 40;
		if (cursor_pos[0] == 0) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_colortext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, colorvaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "The color of the top background,");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "the START border, and the circling dots.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_colortext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, colorvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 1) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_menucolortext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, menucolorvaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "The color of the top border,");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "and the bottom background.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_menucolortext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, menucolorvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 2) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_filenametext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, filenamevaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Shows game filename at the top of the bubble.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_filenametext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, filenamevaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 3) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_locswitchtext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, locswitchvaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "The R button switches the game location");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "between the SD Card and the flashcard.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_locswitchtext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, locswitchvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 4) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_topbordertext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, topbordervaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "The border surrounding the top background.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_topbordertext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, topbordervaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 5) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_countertext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, countervaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "A number of selected game and listed games");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "is shown below the text bubble.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_countertext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, countervaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 6) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_custombottext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, custombotvaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Loads a custom bottom screen image");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "for the game menu.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_custombottext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, custombotvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 7) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_autoupdatetext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, autoupdatevaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Auto-update nds-bootstrap at launch.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_autoupdatetext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, autoupdatevaluetext);
			Ypos += 12;
		}
		if (cursor_pos[0] == 8) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, settings_autodltext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, autodlvaluetext);
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Auto-download the CIA of the latest");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "TWLoader version at launch.");
			Ypos += 12;
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, settings_autodltext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, autodlvaluetext);
			Ypos += 12;
		}
	} else if (subscreenmode == SUBSCREEN_MODE_NTR_TWL) {
		sf2d_draw_texture(shoulderLtex, 0, LshoulderYpos);
		sf2d_draw_texture(shoulderRtex, 248, RshoulderYpos);
		sftd_draw_textf(font, 17, LshoulderYpos+5, RGBA8(0, 0, 0, 255), 11, Lshouldertext);
		sftd_draw_textf(font, 252, RshoulderYpos+5, RGBA8(0, 0, 0, 255), 11, Rshouldertext);

		const char *rainbowledvaluetext = (twlsettings_rainbowledvalue ? "On" : "Off");
		const char *cpuspeedvaluetext = (twlsettings_cpuspeedvalue ? "133mhz (TWL)" : "67mhz (NTR)");
		const char *extvramvaluetext = (twlsettings_extvramvalue ? "On" : "Off");
		const char *bootscreenvaluetext = (twlsettings_bootscreenvalue ? "On" : "Off");
		const char *healthsafetyvaluetext = (twlsettings_healthsafetyvalue ? "On" : "Off");
		const char *resetslot1valuetext = (twlsettings_resetslot1value ? "On" : "Off");

		const char *consolevaluetext;
		switch (twlsettings_consolevalue) {
			case 0:
			default:
				consolevaluetext = "Off";
				break;
			case 1:
				consolevaluetext = "On";
				break;
			case 2:
				consolevaluetext = "On (Debug)";
				break;
		}

		const char *lockarm9scfgextvaluetext = (twlsettings_lockarm9scfgextvalue ? "On" : "Off");

		title = "Settings: NTR/TWL-mode";
		int Ypos = 40;
		if (cursor_pos[1] == 0) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_flashcardtext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Pick a flashcard to use to");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "run ROMs from it.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_flashcardtext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 1) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_rainbowledtext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, rainbowledvaluetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "See rainbow colors glowing in");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "the Notification LED.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_rainbowledtext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, rainbowledvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 2) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_cpuspeedtext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, cpuspeedvaluetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Set to TWL to get rid of lags in some games.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_cpuspeedtext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, cpuspeedvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 3) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_extvramtext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, extvramvaluetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Allows 8 bit VRAM writes");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "and expands the bus to 32 bit.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_extvramtext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, extvramvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 4) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_bootscreentext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, bootscreenvaluetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Displays the DS/DSi boot animation");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "before launched game.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_bootscreentext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, bootscreenvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 5) {
			sftd_draw_textf(font, Xpos+16, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_healthsafetytext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, healthsafetyvaluetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Displays the Health and Safety");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "message on the bottom screen.");
		} else {
			sftd_draw_textf(font, Xpos+16, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_healthsafetytext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, healthsafetyvaluetext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 6) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_resetslot1text);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, resetslot1valuetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Enable this if Slot-1 carts are stuck");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "on white screens.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_resetslot1text);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, resetslot1valuetext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 7) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_consoletext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, consolevaluetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Displays some text before launched game.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_consoletext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, consolevaluetext);
			Ypos += 12;
		}
		if (cursor_pos[1] == 8) {
			sftd_draw_textf(font, Xpos, Ypos, SET_ALPHA(color_data->color, 255), 12, twlsettings_lockarm9scfgexttext);
			sftd_draw_textf(font, XposValue, Ypos, SET_ALPHA(color_data->color, 255), 12, lockarm9scfgextvaluetext);
			Ypos += 12;
			sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, "Locks the ARM9 SCFG_EXT,");
			sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, "avoiding conflict with recent libnds.");
		} else {
			sftd_draw_textf(font, Xpos, Ypos, RGBA8(255, 255, 255, 255), 12, twlsettings_lockarm9scfgexttext);
			sftd_draw_textf(font, XposValue, Ypos, RGBA8(255, 255, 255, 255), 12, lockarm9scfgextvaluetext);
			Ypos += 12;
		}
	} else if (subscreenmode == SUBSCREEN_MODE_FLASH_CARD) {
		// Flash card options.
		static const char *const flash_card_options[][6] = {
			{"DSTT", "R4i Gold", "R4i-SDHC (Non-v1.4.x version) (www.r4i-sdhc.com)",
				"R4 SDHC Dual-Core", "R4 SDHC Upgrade", "SuperCard DSONE"},
			{"Original R4", "M3 Simply", " ", " ", " ", " "},
			{"R4iDSN", "R4i Gold RTS", " ", " ", " ", " "},
			{"Acekard 2(i)", "M3DS Real", " ", " ", " ", " "},
			{"Acekard RPG", " ", " ", " ", " ", " "},
			{"Ace 3DS+", "Gateway Blue Card", "R4iTT", " ", " ", " "},
			{"SuperCard DSTWO", " ", " ", " ", " ", " "},
		};

		if (twlsettings_flashcardvalue < 0 || twlsettings_flashcardvalue > 6) {
			twlsettings_flashcardvalue = 0;
		}
		const char *const *fctext = flash_card_options[twlsettings_flashcardvalue];
		title = twlsettings_flashcardtext;
		int Ypos = 40;
		for (int i = 0; i < 6; i++, Ypos += 12) {
			sftd_draw_textf(font, Xpos, Ypos,
				SET_ALPHA(color_data->color, 255), 12, fctext[i]);
		}
		sftd_draw_textf(font, 8, 184, RGBA8(255, 255, 255, 255), 13, settings_lrpicktext);
		sftd_draw_textf(font, 8, 198, RGBA8(255, 255, 255, 255), 13, settings_absavereturn);
	}
	sftd_draw_textf(font, 2, 2, RGBA8(255, 255, 255, 255), 16, title);
}

/**
 * Move the cursor if necessary.
 * @param hDown Key value from hidKeysDown().
 * @return True if the bottom screen needs to be updated.
 */
bool settingsMoveCursor(u32 hDown)
{
	Lshouldertext = "GUI";
	Rshouldertext = "NTR/TWL";

	if (hDown == 0) {
		// Nothing to do here.
		return false;
	}

	if (subscreenmode == SUBSCREEN_MODE_FLASH_CARD) {
		if (hDown & KEY_LEFT && twlsettings_flashcardvalue != 0){
			twlsettings_flashcardvalue--; // Flashcard
			if (dspfirmfound) { sfx_select->play(); }
		} else if (hDown & KEY_RIGHT && twlsettings_flashcardvalue != 6){
			twlsettings_flashcardvalue++; // Flashcard
			if (dspfirmfound) { sfx_select->play(); }
		} else if (hDown & KEY_A || hDown & KEY_B){
			subscreenmode = SUBSCREEN_MODE_NTR_TWL;
			if (dspfirmfound) { sfx_select->play(); }
		}
	} else if (subscreenmode == SUBSCREEN_MODE_NTR_TWL) {
		if (hDown & KEY_A){
			if (cursor_pos[1] == 0) {
				subscreenmode = SUBSCREEN_MODE_FLASH_CARD;
			} else if (cursor_pos[1] == 1) {
				twlsettings_rainbowledvalue++; // Rainbow LED
				if (twlsettings_rainbowledvalue == 2) {
					twlsettings_rainbowledvalue = 0;
				}
			} else if (cursor_pos[1] == 2) {
				twlsettings_cpuspeedvalue++; // CPU speed
				if (twlsettings_cpuspeedvalue == 2) {
					twlsettings_cpuspeedvalue = 0;
				}
			} else if (cursor_pos[1] == 3) {
				twlsettings_extvramvalue++; // VRAM boost
				if (twlsettings_extvramvalue == 2) {
					twlsettings_extvramvalue = 0;
				}
			} else if (cursor_pos[1] == 4) {
				twlsettings_bootscreenvalue++; // Boot screen
				if (twlsettings_bootscreenvalue == 2) {
					twlsettings_bootscreenvalue = 0;
				}
			} else if (cursor_pos[1] == 5) {
				twlsettings_healthsafetyvalue++; // H&S message
				if (twlsettings_healthsafetyvalue == 2) {
					twlsettings_healthsafetyvalue = 0;
				}
			} else if (cursor_pos[1] == 6) {
				twlsettings_resetslot1value++; // Reset Slot-1
				if (twlsettings_resetslot1value == 2) {
					twlsettings_resetslot1value = 0;
				}
			} else if (cursor_pos[1] == 7) {
				twlsettings_consolevalue++; // Console output
				if (twlsettings_consolevalue == 3) {
					twlsettings_consolevalue = 0;
				}
			} else if (cursor_pos[1] == 8) {
				twlsettings_lockarm9scfgextvalue++; // Lock ARM9 SCFG_EXT
				if (twlsettings_lockarm9scfgextvalue == 2) {
					twlsettings_lockarm9scfgextvalue = 0;
				}
			}
			if (dspfirmfound) { sfx_select->play(); }
		} else if ((hDown & KEY_DOWN) && cursor_pos[1] != 8){
			cursor_pos[1]++;
			if (dspfirmfound) { sfx_select->play(); }
		} else if ((hDown & KEY_UP) && cursor_pos[1] != 0){
			cursor_pos[1]--;
			if (dspfirmfound) { sfx_select->play(); }
		} else if (hDown & KEY_L){
			subscreenmode = SUBSCREEN_MODE_FRONTEND;
			if (dspfirmfound) {
				sfx_switch->stop();	// Prevent freezing
				sfx_switch->play();
			}
		} else if (hDown & KEY_B){
			titleboxXmovetimer = 1;
			fadeout = true;
			if (dspfirmfound) {
				// bgm_settings->stop();
				sfx_back->play();
			}
		}
	} else /* if (subscreenmode == SUBSCREEN_MODE_FRONTEND) */ {
		if (hDown & KEY_A || hDown & KEY_RIGHT){
			if (cursor_pos[0] == 0) {
				settings_colorvalue++; // Color
				if (settings_colorvalue == 19) {
					settings_colorvalue = 0;
				}
				LoadColor();
			} else if (cursor_pos[0] == 1) {
				settings_menucolorvalue++; // Menu color
				if (settings_menucolorvalue == 17) {
					settings_menucolorvalue = 0;
				}
				LoadMenuColor();
			} else if (cursor_pos[0] == 2) {
				settings_filenamevalue++; // Show filename
				if (settings_filenamevalue == 2) {
					settings_filenamevalue = 0;
				}
			} else if (cursor_pos[0] == 3) {
				settings_locswitchvalue++; // Game location switcher
				if (settings_locswitchvalue == 2) {
					settings_locswitchvalue = 0;
				}
			} else if (cursor_pos[0] == 4) {
				settings_topbordervalue++; // Top border
				if (settings_topbordervalue == 2) {
					settings_topbordervalue = 0;
				}
			} else if (cursor_pos[0] == 5) {
				settings_countervalue++; // Game counter
				if (settings_countervalue == 2) {
					settings_countervalue = 0;
				}
			} else if (cursor_pos[0] == 6) {
				settings_custombotvalue++; // Custom bottom image
				if (settings_custombotvalue == 2) {
					settings_custombotvalue = 0;
				}
				LoadBottomImage();
			} else if (cursor_pos[0] == 7) {
				settings_autoupdatevalue++; // Enable or disable autoupdate
				if (settings_autoupdatevalue == 3) {
					settings_autoupdatevalue = 0;
				}
			} else if (cursor_pos[0] == 8) {
				settings_autodlvalue++; // Enable or disable autodownload
				if (settings_autodlvalue == 2) {
					settings_autodlvalue = 0;
				}
			}
			if (dspfirmfound) { sfx_select->play(); }
		} if (hDown & KEY_LEFT){
			if (cursor_pos[0] == 0) {
				settings_colorvalue--; // Color
				if (settings_colorvalue == -1) {
					settings_colorvalue = 18;
				}
				LoadColor();
				if (dspfirmfound) { sfx_select->play(); }
			} else if (cursor_pos[0] == 1) {
				settings_menucolorvalue--; // Menu color
				if (settings_menucolorvalue == -1) {
					settings_menucolorvalue = 16;
				}
				LoadMenuColor();
			}
		} else if ((hDown & KEY_DOWN) && cursor_pos[0] != 8){
			cursor_pos[0]++;
			if (dspfirmfound) { sfx_select->play(); }
		} else if ((hDown & KEY_UP) && cursor_pos[0] != 0){
			cursor_pos[0]--;
			if (dspfirmfound) { sfx_select->play(); }
		} else if (hDown & KEY_R){
			subscreenmode = SUBSCREEN_MODE_NTR_TWL;
			if (dspfirmfound) {
				sfx_switch->stop();	// Prevent freezing
				sfx_switch->play();
			}
		} else if (hDown & KEY_X && checkWifiStatus()){
			UpdateBootstrapRelease();
		} else if (hDown & KEY_Y && checkWifiStatus()){
			UpdateBootstrapUnofficial();
		} else if (hDown & KEY_START && checkWifiStatus() && (checkUpdate() == 0)){
			DownloadTWLoaderCIAs();
		} else if (hDown & KEY_B){
			titleboxXmovetimer = 1;
			fadeout = true;
			if (dspfirmfound) {
				// bgm_settings->stop();
				sfx_back->play();
			}
		}
	}

	// Bottom screen needs to be redrawn.
	return true;
}

/**
 * Load the primary color from the configuration.
 */
void LoadColor(void) {
	static const ColorData colors[] = {
		{
			"romfs:/graphics/topbg/0-gray.png",
			"romfs:/graphics/dotcircle/0-gray.png",
			"romfs:/graphics/start_border/0-gray.png",
			(u32)RGBA8(99, 127, 127, 255)
		},
		{
			"romfs:/graphics/topbg/1-brown.png",
			"romfs:/graphics/dotcircle/1-brown.png",
			"romfs:/graphics/start_border/1-brown.png",
			(u32)RGBA8(139, 99, 0, 255)
		},
		{
			"romfs:/graphics/topbg/2-red.png",
			"romfs:/graphics/dotcircle/2-red.png",
			"romfs:/graphics/start_border/2-red.png",
			(u32)RGBA8(255, 0, 0, 255)
		},
		{
			"romfs:/graphics/topbg/3-pink.png",
			"romfs:/graphics/dotcircle/3-pink.png",
			"romfs:/graphics/start_border/3-pink.png",
			(u32)RGBA8(255, 127, 127, 255)
		},
		{
			"romfs:/graphics/topbg/4-orange.png",
			"romfs:/graphics/dotcircle/4-orange.png",
			"romfs:/graphics/start_border/4-orange.png",
			(u32)RGBA8(223, 63, 0, 255)
		},
		{
			"romfs:/graphics/topbg/5-yellow.png",
			"romfs:/graphics/dotcircle/5-yellow.png",
			"romfs:/graphics/start_border/5-yellow.png",
			(u32)RGBA8(215, 215, 0, 255)
		},
		{
			"romfs:/graphics/topbg/6-yellowgreen.png",
			"romfs:/graphics/dotcircle/6-yellowgreen.png",
			"romfs:/graphics/start_border/6-yellowgreen.png",
			(u32)RGBA8(215, 255, 0, 255)
		},
		{
			"romfs:/graphics/topbg/7-green1.png",
			"romfs:/graphics/dotcircle/7-green1.png",
			"romfs:/graphics/start_border/7-green1.png",
			(u32)RGBA8(0, 255, 0, 255)
		},
		{
			"romfs:/graphics/topbg/8-green2.png",
			"romfs:/graphics/dotcircle/8-green2.png",
			"romfs:/graphics/start_border/8-green2.png",
			(u32)RGBA8(63, 255, 63, 255)
		},
		{
			"romfs:/graphics/topbg/9-lightgreen.png",
			"romfs:/graphics/dotcircle/9-lightgreen.png",
			"romfs:/graphics/start_border/9-lightgreen.png",
			(u32)RGBA8(31, 231, 31, 255)
		},
		{
			"romfs:/graphics/topbg/10-skyblue.png",
			"romfs:/graphics/dotcircle/10-skyblue.png",
			"romfs:/graphics/start_border/10-skyblue.png",
			(u32)RGBA8(0, 63, 255, 255)
		},
		{
			"romfs:/graphics/topbg/11-lightblue.png",
			"romfs:/graphics/dotcircle/11-lightblue.png",
			"romfs:/graphics/start_border/11-lightblue.png",
			(u32)RGBA8(63, 63, 255, 255)
		},
		{
			"romfs:/graphics/topbg/12-blue.png",
			"romfs:/graphics/dotcircle/12-blue.png",
			"romfs:/graphics/start_border/12-blue.png",
			(u32)RGBA8(0, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/13-violet.png",
			"romfs:/graphics/dotcircle/13-violet.png",
			"romfs:/graphics/start_border/13-violet.png",
			(u32)RGBA8(127, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/14-purple.png",
			"romfs:/graphics/dotcircle/14-purple.png",
			"romfs:/graphics/start_border/14-purple.png",
			(u32)RGBA8(255, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/15-fuchsia.png",
			"romfs:/graphics/dotcircle/15-fuchsia.png",
			"romfs:/graphics/start_border/15-fuchsia.png",
			(u32)RGBA8(255, 0, 127, 255)
		},
		{
			"romfs:/graphics/topbg/16-red&blue.png",
			"romfs:/graphics/dotcircle/16-red&blue.png",
			"romfs:/graphics/start_border/16-red&blue.png",
			(u32)RGBA8(255, 0, 255, 255)
		},
		{
			"romfs:/graphics/topbg/17-green&yellow.png",
			"romfs:/graphics/dotcircle/17-green&yellow.png",
			"romfs:/graphics/start_border/17-green&yellow.png",
			(u32)RGBA8(215, 215, 0, 255)
		},
		{
			"romfs:/graphics/topbg/18-christmas.png",
			"romfs:/graphics/dotcircle/18-christmas.png",
			"romfs:/graphics/start_border/18-christmas.png",
			(u32)RGBA8(255, 255, 0, 255)
		},
	};

	if (settings_colorvalue < 0 || settings_colorvalue > 18)
		settings_colorvalue = 0;
	color_data = &colors[settings_colorvalue];
	LogFM("LoadColor()", "Colors load successfully");
}

/**
 * Load the menu color from the configuration.
 */
void LoadMenuColor(void) {
	static const u32 menu_colors[] = {
		(u32)RGBA8(255, 255, 255, 255),		// White
		(u32)RGBA8(63, 63, 63, 195),		// Black
		(u32)RGBA8(139, 99, 0, 195),		// Brown
		(u32)RGBA8(255, 0, 0, 195),		// Red
		(u32)RGBA8(255, 163, 163, 195),		// Pink
		(u32)RGBA8(255, 127, 0, 223),		// Orange
		(u32)RGBA8(255, 255, 0, 223),		// Yellow
		(u32)RGBA8(215, 255, 0, 223),		// Yellow-Green
		(u32)RGBA8(0, 255, 0, 223),		// Green 1
		(u32)RGBA8(95, 223, 95, 193),		// Green 2
		(u32)RGBA8(127, 231, 127, 223),		// Light Green
		(u32)RGBA8(63, 127, 255, 223),		// Sky Blue
		(u32)RGBA8(127, 127, 255, 223),		// Light Blue
		(u32)RGBA8(0, 0, 255, 195),		// Blue
		(u32)RGBA8(127, 0, 255, 195),		// Violet
		(u32)RGBA8(255, 0, 255, 195),		// Purple
		(u32)RGBA8(255, 63, 127, 195),		// Fuchsia
	};

	if (settings_menucolorvalue < 0 || settings_menucolorvalue > 16)
		settings_menucolorvalue = 0;
	menucolor = menu_colors[settings_menucolorvalue];
	LogFM("LoadMenuColor()", "Menu color load successfully");
}

/**
 * Load the filename of the bottom screen image.
 */
void LoadBottomImage() {
	bottomloc = "romfs:/graphics/bottom.png";

	if (settings_custombotvalue == 1) {
		if( access( "sdmc:/_nds/twloader/bottom.png", F_OK ) != -1 ) {
			bottomloc = "sdmc:/_nds/twloader/bottom.png";
			LogFM("LoadBottomImage()", "Using custom bottom image. Method load successfully");
		} else {
			bottomloc = "romfs:/graphics/bottom.png";
			LogFM("LoadBottomImage()", "Using default bottom image. Method load successfully");
		}
	}
}
