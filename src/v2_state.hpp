#pragma once
#include "analysis.hpp"
#include "cargo.hpp"
#include "volume.hpp"
#include "water_fill.hpp"
#include "bed_dimensions.hpp"
#include "water_loaded.hpp"
#include "virtual_tarp.hpp"
#include "water_preview.hpp"
#include "registration.hpp"
#include "png.hpp"
#include <deque>
#include <optional>
#include <atomic>
constexpr int ACTIVE_FILE=120,BASE_FILE=121,COMPARE=122,REGION_SELECT=123,REGION_CLEAR=124,OPTIONS=125,PNG_SAVE=126,REPORT_SAVE=127,CLOSE_FILE=128,OVERLAY=129,CARGO=130,AUTO_ALIGN=131,CONTEXT_POINTS=132,WATER=133,LOAD_DATABASE=134,UNLOAD_DATABASE=135,APPEARANCE=136,MANUAL_CALC=137;
struct Document {std::filesystem::path path;Model data;};
std::vector<Document> documents;int activeDocument=-1,baseDocument=-1;
std::deque<std::filesystem::path> loadQueue;
Region region;CompareOptions compareOptions;std::optional<Comparison> comparison;
bool differenceView=false,overlayView=false,selectRegion=false,selecting=false,replacing=false,v2TestMode=false;
GLuint baseList=0,contextList=0;bool showContext=false;
BedDimensions bedDimensions;
WaterFill water;bool showWater=false;GLuint waterList=0;
WaterFill fullWater;GLuint fullWaterList=0,fullLayerList=0,cargoLayerList=0,cargoLayerFaces=0;
VirtualTarp tarp;GLuint tarpPoints=0,tarpFaces=0;
struct ScanLayers {bool enabled=false,full=false,empty=false,cargo=true,other=false,emptyWater=false,fullWater=false,label=true,tarp=false;};
ScanLayers scanLayers;
bool showZone=true;
std::wstring waterSummary();
Cargo cargo;bool cargoView=false;double cargoThreshold=50;bool cargoLargest=true,cargoClean=true;
struct ViewBounds{Point lo{},hi{};bool valid=false;};
std::optional<ViewBounds> lockedFrame;
POINT selectionStart{},selectionEnd{};
void invalidateComparison(){lockedFrame.reset();comparison.reset();differenceView=false;overlayView=false;cargoView=false;cargo={};water={};bedDimensions={};fullWater={};tarp={};showWater=false;scanLayers.enabled=false;}
void selectDocument(int index);
void acceptLoaded(Model next);
void enqueueFiles(const std::vector<std::filesystem::path>& paths);
void v2Command(int id,int notification=0);
void createV2Controls(HINSTANCE inst);
void v2Layout();
void applyRegion(const Region& r);
void finishRegion();
std::wstring comparisonSummary();

std::future<registration::Result> alignmentTask;bool calculateAfterAlignment=false;
std::atomic<int> alignmentProgress{0};int shownAlignmentProgress=-1;
void finishAlignment();
struct ViewerVolumeResult {VolumeResult volume;WaterFill water,fullWater;VirtualTarp tarp;BedDimensions dimensions;};
std::future<ViewerVolumeResult> volumeTask;
std::atomic<int> volumeProgress{0};int shownVolumeProgress=-1;
void finishComparison();

#include "control_database.hpp"
control::Database controlDatabase;
bool lightTheme=false;enum class ReportFormat {HTML,PDF};ReportFormat reportFormat=ReportFormat::HTML;
void drawControlComparison();
void drawBedDimensions();
