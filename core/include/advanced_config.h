/*
 * advanced_config.h - Stub for kicad-core standalone build
 *
 * Provides default configuration values without wx dependencies
 */

#ifndef KICAD_CORE_ADVANCED_CONFIG_H
#define KICAD_CORE_ADVANCED_CONFIG_H

// Stub ADVANCED_CFG class with default values
class ADVANCED_CFG {
public:
    // Triangulation settings (defaults from KiCad)
    int m_TriangulateSimplificationLevel = 50;
    double m_TriangulateMinimumArea = 1000.0;

    // Other settings that might be accessed
    bool m_EnableLibWithText = false;
    bool m_EnableEeschemaPrintCairo = false;
    int m_UpdateUIEventInterval = 0;
    double m_DrawArcAccuracy = 0.005;
    double m_DrawArcCenterMaxAngle = 50.0;
    int m_MaxUndoItems = 0;
    double m_MinPlotPenWidth = 0.0;
    int m_3DRT_BevelExtentFactor = 1;
    int m_3DRT_BevelHeight_um = 30;
    bool m_ShowRepairSchematic = false;
    bool m_ShowPropertiesPanel = true;
    bool m_ShowEventCounters = false;
    bool m_AllowManualCanvasScale = false;
    double m_CanvasScale = 1.0;
    bool m_CompactSave = false;
    int m_CoroutineStackSize = 0;
    int m_DrawBoundingBoxes = 0;
    bool m_ShowPcbnewExportNetlist = false;
    bool m_Skip3DModelFileCache = false;
    bool m_Skip3DModelMemoryCache = false;
    bool m_HideVersionFromTitle = false;
    bool m_TraceMasks = false;
    bool m_ShowRouterDebugGraphics = false;
    bool m_ExtraZoneDisplayModes = false;
    double m_MinClrDistance = 0.0;
    bool m_DebugZoneFiller = false;
    bool m_DebugPDFWriter = false;
    int m_HotkeysDumper = 0;
    bool m_DrawTriangulationOutlines = false;
    bool m_StrokeTriangulation = false;
    bool m_ExtraClearance = false;
    double m_SmallDrillMarkSize = 0.0;
    int m_HoleijWallThickness = 0;
    int m_MaxTangentAngleDeviation = 1;
    int m_MaxClearanceDistanceFactor = 2;
    int m_ViaijFillMinRatio = 0;
    bool m_RealtimeConnectivity = true;
    bool m_EnableCacheFriendlyFracture = true;
    double m_FontErrorSize = 0.0;
    double m_OcctVerbosity = 0.0;
    bool m_Use3DConnexionDriver = false;
    bool m_IncrementalConnectivity = true;

    static ADVANCED_CFG& GetCfg() {
        static ADVANCED_CFG instance;
        return instance;
    }

private:
    ADVANCED_CFG() = default;
};

#endif // KICAD_CORE_ADVANCED_CONFIG_H
