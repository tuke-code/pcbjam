/**
 * Test-harness stub of the EDA_3D_VIEWER_SETTINGS / APP_SETTINGS_BASE /
 * JSON_SETTINGS chain. The renderer only reads m_Render/m_Camera plain
 * fields; none of the JSON load/store machinery ever runs, so every virtual
 * is a no-op and the ctor fills the render settings with the upstream
 * defaults (deterministic test values, documented deviations flagged).
 */

#include "kicad_stubs_3d.h"

#include "3d_viewer/eda_3d_viewer_settings.h"
#include "common_ogl/ogl_attr_list.h" // ANTIALIASING_MODE (fwd-declared in the settings header)

#include <settings/json_settings_internals.h>

// ---- JSON_SETTINGS ----

JSON_SETTINGS::JSON_SETTINGS( const wxString& aFilename, SETTINGS_LOC aLocation,
                              int aSchemaVersion, bool aCreateIfMissing, bool aCreateIfDefault,
                              bool aWriteFile ) :
        m_filename( aFilename ),
        m_legacy_filename( "" ),
        m_location( aLocation ),
        m_createIfMissing( aCreateIfMissing ),
        m_createIfDefault( aCreateIfDefault ),
        m_writeFile( aWriteFile ),
        m_modified( false ),
        m_deleteLegacyAfterMigration( false ),
        m_resetParamsIfMissing( true ),
        m_schemaVersion( aSchemaVersion ),
        m_manager( nullptr )
{
    m_internals = std::make_unique<JSON_SETTINGS_INTERNALS>();
}

JSON_SETTINGS::~JSON_SETTINGS() = default;

void JSON_SETTINGS::Load() {}
bool JSON_SETTINGS::Store() { return false; }
bool JSON_SETTINGS::LoadFromFile( const wxString& ) { return false; }
bool JSON_SETTINGS::SaveToFile( const wxString&, bool ) { return false; }
std::map<std::string, nlohmann::json> JSON_SETTINGS::GetFileHistories() { return {}; }
bool JSON_SETTINGS::MigrateFromLegacy( wxConfigBase* ) { return false; }

// ---- APP_SETTINGS_BASE ----

APP_SETTINGS_BASE::APP_SETTINGS_BASE( const std::string& aFilename, int aSchemaVersion ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::USER, aSchemaVersion, true, true, true ),
        m_CrossProbing(),
        m_FindReplace(),
        m_Graphics(),
        m_ColorPicker(),
        m_LibTree(),
        m_Printing(),
        m_SearchPane(),
        m_System(),
        m_Window(),
        m_appSettingsSchemaVersion( aSchemaVersion )
{
}

bool APP_SETTINGS_BASE::MigrateFromLegacy( wxConfigBase* ) { return false; }

// ---- EDA_3D_VIEWER_SETTINGS ----

EDA_3D_VIEWER_SETTINGS::EDA_3D_VIEWER_SETTINGS() :
        APP_SETTINGS_BASE( "3d_viewer", 0 ),
        m_Render(),
        m_Camera()
{
    RENDER_SETTINGS& r = m_Render;

    // Upstream defaults (eda_3d_viewer_settings.cpp PARAM defaults), with two
    // determinism-driven deviations: engine is OPENGL (the renderer under
    // test) and AA is NONE (context is single-sample anyway).
    r.engine = RENDER_ENGINE::OPENGL;
    r.grid_type = GRID3D_TYPE::NONE;
    r.opengl_AA_mode = ANTIALIASING_MODE::AA_NONE;
    r.material_mode = MATERIAL_MODE::NORMAL;

    r.opengl_AA_disableOnMove = false;
    r.opengl_thickness_disableOnMove = false;
    r.opengl_microvias_disableOnMove = false;
    r.opengl_holes_disableOnMove = false;
    r.opengl_render_bbox_only_OnMove = false;
    r.opengl_copper_thickness = true;
    r.show_model_bbox = false;
    r.show_off_board_silk = false;
    r.highlight_on_rollover = false;
    r.opengl_selection_color = KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 );

    r.raytrace_anti_aliasing = false;
    r.raytrace_backfloor = false;
    r.raytrace_post_processing = false;
    r.raytrace_procedural_textures = false;
    r.raytrace_reflections = false;
    r.raytrace_refractions = false;
    r.raytrace_shadows = false;
    r.raytrace_nrsamples_shadows = 0;
    r.raytrace_nrsamples_reflections = 0;
    r.raytrace_nrsamples_refractions = 0;
    r.raytrace_spread_shadows = 0.0f;
    r.raytrace_spread_reflections = 0.0f;
    r.raytrace_spread_refractions = 0.0f;
    r.raytrace_recursivelevel_reflections = 0;
    r.raytrace_recursivelevel_refractions = 0;
    r.raytrace_lightColorCamera = KIGFX::COLOR4D( 0.2, 0.2, 0.2, 1.0 );
    r.raytrace_lightColorTop = KIGFX::COLOR4D( 0.247, 0.247, 0.247, 1.0 );
    r.raytrace_lightColorBottom = KIGFX::COLOR4D( 0.247, 0.247, 0.247, 1.0 );

    r.show_adhesive = true;
    r.show_navigator = false;
    r.show_board_body = true;
    r.show_plated_barrels = true;
    r.show_comments = true;
    r.show_drawings = true;
    r.show_eco1 = true;
    r.show_eco2 = true;

    for( bool& user : r.show_user )
        user = false;

    r.show_footprints_insert = true;
    r.show_footprints_normal = true;
    r.show_footprints_virtual = true;
    r.show_footprints_not_in_posfile = true;
    r.show_footprints_dnp = true;
    r.show_silkscreen_top = true;
    r.show_silkscreen_bottom = true;
    r.show_soldermask_top = true;
    r.show_soldermask_bottom = true;
    r.show_solderpaste = true;
    r.show_copper_top = true;
    r.show_copper_bottom = true;
    r.show_zones = true;
    r.show_fp_references = true;
    r.show_fp_values = true;
    r.show_fp_text = true;
    r.subtract_mask_from_silk = false;
    r.clip_silk_on_via_annuli = true;
    r.differentiate_plated_copper = true;
    r.use_board_editor_copper_colors = false;
    r.preview_show_board_body = true;

    m_Camera.animation_enabled = false;
    m_Camera.moving_speed_multiplier = 3;
    m_Camera.rotation_increment = 10.0;
    m_Camera.projection_mode = 0;
}

LAYER_PRESET_3D* EDA_3D_VIEWER_SETTINGS::FindPreset( const wxString& )
{
    return nullptr;
}

bool EDA_3D_VIEWER_SETTINGS::MigrateFromLegacy( wxConfigBase* )
{
    return false;
}
