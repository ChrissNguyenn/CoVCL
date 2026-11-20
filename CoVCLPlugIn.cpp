#include "pch.h"
#include "CoVCLPlugIn.h"
#include "RadarScreen.h"



CoVCLPlugIn::CoVCLPlugIn(void):CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, MY_PLUGIN_NAME, MY_PLUGIN_VERSION, MY_PLUGIN_DEVELOPER, MY_PLUGIN_COPYRIGHT)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    char DllPathFile[_MAX_PATH];

    GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
    DllPath = DllPathFile;
    DllPath.resize(DllPath.size() - strlen("CoVCL.dll"));
    
    LoadConfigFile();

    Stca = new CSTCA(CoVCLConfig);

    // Register tag items
    RegisterTagItemType("Two number ground speed", CoVCLTags::GS);
    RegisterTagItemType("Vertical Indicator", CoVCLTags::VZ_INDICATOR);
    RegisterTagItemType("Dynamic Coordinated Point (reduced)", CoVCLTags::COPXN_POINT_REDUCED);
    RegisterTagItemType("Dynamic Coordinated Level (reduced)", CoVCLTags::COPXN_ALT_REDUCED);
    RegisterTagItemType("Dynamic Coordinated Level (full)", CoVCLTags::COPXN_ALT_FULL);

    RegisterTagItemType("Dummy item", CoVCLTags::DUMMY);
    RegisterTagItemType("Dummy item (tagged)", CoVCLTags::DUMMY_TAGGED);

    RegisterTagItemType("Scratchpad Indicator (Detail)", CoVCLTags::SCRATCHPAD_INDIC);

    RegisterTagItemType("Conflict group (Detail)", CoVCLTags::CONFLICT_GROUP_DETAIL);
    RegisterTagItemType("Conflict group (Tagged)", CoVCLTags::CONFLICT_GROUP_TAGGED);

    RegisterTagItemType("RFL", CoVCLTags::RFL);

    RegisterTagItemType("CFL", CoVCLTags::CFL);
    RegisterTagItemType("CFL (Detail)", CoVCLTags::CFL_DETAILED);

    RegisterTagItemType("Approach Intention Code", CoVCLTags::APP_INTENTION);

    RegisterTagItemType("Vertical Speed", CoVCLTags::VZ);

    RegisterTagItemType("STCA Indicator", CoVCLTags::STCA);

    RegisterTagItemType("OCL Flag", CoVCLTags::OCL_FLAG);

    RegisterTagItemType("Assigned Speed", CoVCLTags::ASSIGNED_SPEED);

    RegisterTagItemFunction("Assign Conflict Group", CoVCLTags::FUNCTION_CONFLICT_POPUP);
    //RegisterTagItemFunction("Show OCL", CoVCLTags::FUNCTION_OCL_TP);

    RegisterTagItemFunction("Open ASP Popup", CoVCLTags::FUNCTION_OPEN_ASP);

    RegisterTagItemType("Stand", CoVCLTags::STAND);

    DisplayUserMessage("Message", "CoVCL PlugIn", string("Version " + string(MY_PLUGIN_VERSION) + " loaded.").c_str(), false, false, false, false, false);
}

CoVCLPlugIn::~CoVCLPlugIn()
{
    GdiplusShutdown(gdiplusToken);
}

CRadarScreen* CoVCLPlugIn::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{
    if (CanLoadRadarScreen)
        return new RadarScreen(this);
    else
        return NULL;
}

bool CoVCLPlugIn::OnCompileCommand(const char* sCommandLine)
{
    if (strcmp(sCommandLine, ".covcl reload") == 0) {
        LoadConfigFile();
        return true;
    }

    if (strcmp(sCommandLine, ".covcl reload-local") == 0 || strcmp(sCommandLine, ".covrl") == 0) {
        LoadConfigFile(false);
        return true;
    }

    if (strcmp(sCommandLine, ".covcl perf") == 0) {
        performanceMode = true;
        return true;
    }

     if (strcmp(sCommandLine, ".covcl debug") == 0) {
        Debug = !Debug;
        if (Debug) {
            DisplayUserMessage("CoVCL", "Debug", "Debug enabled", true, true, false, true, false);
        }
        else {
            DisplayUserMessage("CoVCL", "Debug", "Debug disabled", true, true, false, true, false);
        }
        return true;
    }

    return false;
}

void CoVCLPlugIn::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
    if (ItemCode == CoVCLTags::GS) {
        if (!RadarTarget.IsValid())
            return;

        int gs = roundUp(RadarTarget.GetPosition().GetReportedGS(), 10);
        string gs_s = padWithZeros(3, gs);
        strcpy_s(sItemString, 16, gs_s.substr(0, 2).c_str());
    }

    if (ItemCode == CoVCLTags::SCRATCHPAD_INDIC) {
        string txt = CoVCLCharacters::Pen;
        if (strlen(FlightPlan.GetControllerAssignedData().GetScratchPadString()) > 0)
            txt = FlightPlan.GetControllerAssignedData().GetScratchPadString();

        strcpy_s(sItemString, 16, txt.substr(0, 15).c_str());
    }

    if (ItemCode == CoVCLTags::CONFLICT_GROUP_DETAIL) {
        if (ConflictGroups.find(FlightPlan.GetCallsign()) != ConflictGroups.end()) {
            strcpy_s(sItemString, 16, ConflictGroups[FlightPlan.GetCallsign()].c_str());
            *pColorCode = TAG_COLOR_RGB_DEFINED;
            *pRGB = GetConflictGroupColor(this->CoVCLConfig, ConflictGroups[FlightPlan.GetCallsign()].c_str());
        }
        else {
            strcpy_s(sItemString, 16, CoVCLCharacters::Moon.c_str());
        }
    }

    if (ItemCode == CoVCLTags::CONFLICT_GROUP_TAGGED) {
        if (ConflictGroups.find(FlightPlan.GetCallsign()) != ConflictGroups.end()) {
            strcpy_s(sItemString, 16, ConflictGroups[FlightPlan.GetCallsign()].c_str());
            *pColorCode = TAG_COLOR_RGB_DEFINED;
            *pRGB = GetConflictGroupColor(this->CoVCLConfig, ConflictGroups[FlightPlan.GetCallsign()].c_str());
        }
        else {
            strcpy_s(sItemString, 16, "");
        }
    }

    if (ItemCode == CoVCLTags::DUMMY) {
        DetailedAircraft = RadarTarget.GetCallsign();

        strcpy_s(sItemString, 16, "");
    }


    if (ItemCode == CoVCLTags::ASSIGNED_SPEED) {
        string aspeed = "";
        if (FlightPlan.GetControllerAssignedData().GetAssignedMach() != 0) {
            aspeed = "m";

            aspeed += "." + to_string(FlightPlan.GetControllerAssignedData().GetAssignedMach());
            if (string(FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(CoVCLTags::ANNOTATION_SPEED_SIGN)) == string("+"))
                aspeed += "+";
            if (string(FlightPlan.GetControllerAssignedData().GetFlightStripAnnotation(CoVCLTags::ANNOTATION_SPEED_SIGN)) == string("-"))
                aspeed += "-";
        } else if (FlightPlan.GetControllerAssignedData().GetAssignedSpeed() != 0) {
            aspeed = "k";
            if (FlightPlan.GetControllerAssignedData().GetAssignedSpeed() % 10 == 1)
                aspeed += to_string((FlightPlan.GetControllerAssignedData().GetAssignedSpeed() - 1)/10) + "+";
            else if (FlightPlan.GetControllerAssignedData().GetAssignedSpeed() % 10 == 9)
                aspeed += to_string((FlightPlan.GetControllerAssignedData().GetAssignedSpeed() + 1)/10) + "-";
            else
                aspeed += to_string(FlightPlan.GetControllerAssignedData().GetAssignedSpeed()/10);
        }

        strcpy_s(sItemString, 16, aspeed.c_str());
    }

    if (ItemCode == CoVCLTags::DUMMY_TAGGED) {
        if (DetailedAircraft == RadarTarget.GetCallsign())
            DetailedAircraft = "";

        strcpy_s(sItemString, 16, "");
    }

    if (ItemCode == CoVCLTags::RFL) {
        string rfl = "RFL";
        if (FlightPlan.IsValid()) {
            if (FlightPlan.GetFinalAltitude() > 0) {
                rfl = padWithZeros(2, FlightPlan.GetFinalAltitude() / 1000);

                // VFRs can be cleared up to 500 feet interval so we have to display more
                if (FlightPlan.GetFlightPlanData().GetPlanType() == "V")
                    rfl = padWithZeros(3, FlightPlan.GetFinalAltitude() / 100);

                if (FlightPlan.GetFinalAltitude() <= GetTransitionAltitude())
                    rfl = "A" + rfl;
            }
        }

        strcpy_s(sItemString, 16, rfl.c_str());
    }

    if (ItemCode == CoVCLTags::CFL || ItemCode == CoVCLTags::CFL_DETAILED) {
        string cfl = "";
        if (ItemCode == CoVCLTags::CFL_DETAILED)
            cfl = "CFL";

        if (FlightPlan.IsValid()) {
            if (FlightPlan.GetClearedAltitude() > 0) {
                cfl = padWithZeros(2, FlightPlan.GetClearedAltitude() / 1000);

                // VFRs can be cleared up to 500 feet interval so we have to display more
                if (FlightPlan.GetFlightPlanData().GetPlanType() == "V")
                    cfl = padWithZeros(3, FlightPlan.GetClearedAltitude() / 100);

                if (FlightPlan.GetClearedAltitude() <= GetTransitionAltitude())
                    cfl = "A" + cfl;
            }

            if (FlightPlan.GetControllerAssignedData().GetClearedAltitude() == 1)
                cfl = CoVCLCharacters::ClearedAPPIndicator.c_str();;

            if (FlightPlan.GetControllerAssignedData().GetClearedAltitude() == 2)
                cfl = CoVCLCharacters::ClearedVisualIndicator.c_str();

            // If not detailed and reached alt, then nothing to show
            if (ItemCode == CoVCLTags::CFL && abs(RadarTarget.GetPosition().GetFlightLevel() - FlightPlan.GetClearedAltitude()) < 100)
                cfl = "";
        }

        strcpy_s(sItemString, 16, cfl.c_str());
    }

    if (ItemCode == CoVCLTags::VZ_INDICATOR) {
        if (!RadarTarget.IsValid())
            return;

        string tendency = "-";
        int delta_fl = RadarTarget.GetPosition().GetFlightLevel() -
            RadarTarget.GetPreviousPosition(RadarTarget.GetPosition()).GetFlightLevel();
        if (abs(delta_fl) >= 10)
            tendency = delta_fl < 0 ? "|" : "^";

        strcpy_s(sItemString, 16, tendency.c_str());
    }

    if (ItemCode == CoVCLTags::APP_INTENTION) {
        // Three types of codes:
        // 1. VFR
        // 2. Arrival (Assigned Runway with different colours)
        // 3. Departure (Abbreviated SID)
        
        if (FlightPlan.IsValid()) {

            if (FlightPlan.GetFlightPlanData().GetPlanType()[0] == 'V') {
                // We have a VFR flight
                *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                auto element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "intention_code_vfr");
                *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                strcpy_s(sItemString, 16, "VFR");
            }
            else {

                // We have a departure
                if (FlightPlan.GetDistanceFromOrigin() <= 50) {
                    string abbr_sid = "";
                    if (strlen(FlightPlan.GetFlightPlanData().GetSidName()) > 0) {
                        abbr_sid = FlightPlan.GetFlightPlanData().GetSidName();
                        abbr_sid = abbr_sid.substr(0, 3);
                    }

                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                    auto element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "intention_code_departure");
                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                    strcpy_s(sItemString, 16, abbr_sid.c_str());
                }
                else {
                    // We have an arrival
                    string arr_rwy = "";
                    if (strlen(FlightPlan.GetFlightPlanData().GetArrivalRwy()) > 0) {
                        arr_rwy = FlightPlan.GetFlightPlanData().GetArrivalRwy();
                        arr_rwy = arr_rwy.substr(0, 3);
                    }

                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                    auto element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "intention_code_arrival");
                    
                    // Per-airport arrival runway colours (VCL: VVTS and VVNB parallel runways), see [[arrival_runway_colour]] in config.toml
                    if (CoVCLConfig.contains("arrival_runway_colour")) {
                        for (const auto& rule : toml::find<toml::array>(CoVCLConfig, "arrival_runway_colour")) {
                            if (!startsWith(toml::find<string>(rule, "airport").c_str(), FlightPlan.GetFlightPlanData().GetDestination()))
                                continue;
                            bool match = false;
                            for (const auto& rwy : toml::find<vector<string>>(rule, "runways")) {
                                if (startsWith(rwy.c_str(), FlightPlan.GetFlightPlanData().GetArrivalRwy()))
                                    match = true;
                            }
                            if (match) {
                                element_colour = toml::find<std::vector<int>>(rule, "colour");
                                break;
                            }
                        }
                    }

                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                    strcpy_s(sItemString, 16, arr_rwy.c_str());
                }
            }

        }

    }

    if (ItemCode == CoVCLTags::VZ) {
        if (!RadarTarget.IsValid())
            return;

        string VerticalRate = "00";
        CRadarTargetPositionData pos = RadarTarget.GetPosition();
        CRadarTargetPositionData oldpos = RadarTarget.GetPreviousPosition(pos);
        int mathVerticalRate = 0;
        if (pos.IsValid() && oldpos.IsValid()) {
            int deltaalt = pos.GetFlightLevel() - oldpos.GetFlightLevel();
            int deltaT = oldpos.GetReceivedTime() - pos.GetReceivedTime();

            if (deltaT > 0) {
                float vz = abs(deltaalt) * (60.0f / deltaT);
                mathVerticalRate = (int)vz;

                // If the rate is too much
                if ((int)abs(vz) >= 9999) {
                    VerticalRate = "++";
                    if (deltaalt < 0)
                        VerticalRate = "--";
                }
                else if (abs(vz) >= 100 && abs(deltaalt) >= 20) {
                    string rate = padWithZeros(2, (int)abs(vz / 100));
                    VerticalRate = "+" + rate;

                    if (deltaalt < 0)
                        VerticalRate = "-" + rate;
                }
            }
        }

        strcpy_s(sItemString, 16, VerticalRate.c_str());
    }
    
    if (ItemCode == CoVCLTags::COPXN_POINT_REDUCED) {
        if (!FlightPlan.IsValid())
            return;

        string copx = "";

        if (FlightPlan.GetTrackingControllerIsMe()) {
            if (strlen(FlightPlan.GetExitCoordinationPointName()) > 0) {
                copx = FlightPlan.GetExitCoordinationPointName();
                *pColorCode = GetCoordinationTagColour(FlightPlan.GetExitCoordinationNameState());
            }
        }
        else {
            if (strlen(FlightPlan.GetEntryCoordinationPointName()) > 0) {
                copx = FlightPlan.GetEntryCoordinationPointName();
                *pColorCode = GetCoordinationTagColour(FlightPlan.GetEntryCoordinationPointState());
            }
                
        }

        if (string(FlightPlan.GetControllerAssignedData().GetDirectToPointName()) == copx) {\
            // If direct to COPX has been given, no need to display it
            copx = "";
        }
        
        strcpy_s(sItemString, 16, copx.substr(0, 15).c_str());
    }

    if (ItemCode == CoVCLTags::COPXN_ALT_REDUCED || ItemCode == CoVCLTags::COPXN_ALT_FULL) {
        if (!FlightPlan.IsValid())
            return;

        string copx_alt = "";

        if (FlightPlan.GetTrackingControllerIsMe()) {
            if (FlightPlan.GetExitCoordinationAltitude() > 0) {
                copx_alt = string("x") + padWithZeros(2, FlightPlan.GetExitCoordinationAltitude() / 1000);
                *pColorCode = GetCoordinationTagColour(FlightPlan.GetExitCoordinationAltitudeState());
            }
        }
        else if (FlightPlan.GetEntryCoordinationAltitude() > 0) {
            copx_alt = string("e") + padWithZeros(2, FlightPlan.GetEntryCoordinationAltitude() / 1000);
            *pColorCode = GetCoordinationTagColour(FlightPlan.GetEntryCoordinationAltitudeState());
        }

        if (ItemCode == CoVCLTags::COPXN_ALT_FULL && copx_alt.size() == 0) {
            if (FlightPlan.GetTrackingControllerIsMe())
                copx_alt = "x...";
            else
                copx_alt = "e...";
        }


        strcpy_s(sItemString, 16, copx_alt.substr(0, 15).c_str());
    }

    if (ItemCode == CoVCLTags::STCA) {
        if (Stca->IsSTCA(RadarTarget.GetCallsign())) {

            *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
            auto element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "sep_warning");
            if (Blink)
                element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "stca_warning");
            
            *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

            strcpy_s(sItemString, 16, "STCA");
        }
        else {
            strcpy_s(sItemString, 16, "");
        }
    }

    if (ItemCode == CoVCLTags::OCL_FLAG) {
        if (!FlightPlan.IsValid())
            return;
        string test = FlightPlan.GetFlightPlanData().GetRoute();
        const char* destination = FlightPlan.GetFlightPlanData().GetDestination();
        // if is OCL revelant
        if (StringContainsArray(test, Brest_Oceanic_Points)) {
            // Display OCL flag
            if (HasOCL(FlightPlan.GetCallsign())) {
                // Check if level change required

                int ocl_level = GetOCLLevel(FlightPlan.GetCallsign());

                int cfl = FlightPlan.GetControllerAssignedData().GetClearedAltitude();
                if (cfl == 0)
                    cfl = FlightPlan.GetControllerAssignedData().GetFinalAltitude();

                if (cfl != ocl_level && ocl_level != 0) {
                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
                    auto element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "intention_code_departure");
                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);

                    string item = "LCHG";
                    item += std::to_string(ocl_level / 1000);

                    strcpy_s(sItemString, 16, item.c_str());
                }
            }
            // If there is no OCL and exit of sector is within 45mins and the flight is headed to the Americas (ICAOs starting with KCPSTMN, and SPEM) we display an alert
            else if (startsWith("K", destination) || startsWith("C", destination) || 
            startsWith("P", destination) || startsWith("T", destination) || 
            startsWith("S", destination) || startsWith("M", destination) || 
            startsWith("N", destination) || startsWith("LFVP", destination) || startsWith("LFVM", destination)) {
                int minutes = 99;
                for (int i = FlightPlan.GetExtractedRoute().GetPointsCalculatedIndex(); i < FlightPlan.GetExtractedRoute().GetPointsNumber(); i++) {
                    if (StringContainsArray(FlightPlan.GetExtractedRoute().GetPointName(i), Brest_Oceanic_Points)) {
                        minutes = FlightPlan.GetExtractedRoute().GetPointDistanceInMinutes(i);
                        break;
                    }
                }

                if (minutes <= 30) {
                    *pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;

                    auto element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "intention_code_departure");
                    if (minutes < 15)
                        element_colour = toml::find<std::vector<int>>(CoVCLConfig, "colours", "sep_warning");

                    *pRGB = RGB(element_colour[0], element_colour[1], element_colour[2]);
                    strcpy_s(sItemString, 16, "OCL");
                }
            }
        }
        else {
            strcpy_s(sItemString, 16, "");
        }
    }

    if (ItemCode == CoVCLTags::STAND) {
        if (!FlightPlan.IsValid())
            return;

        strcpy_s(sItemString, 16, "INOP");
    }
}

void CoVCLPlugIn::OnTimer(int Counter)
{
    if (RawOCLData.valid() && RawOCLData.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        string d = RawOCLData.get();

        try {
            SharedData::OCLData = nlohmann::json::parse(d);
        }
        catch (std::exception &exc) {

        }
    }

    
    Blink = !Blink;
    
    // STCA gets refreshed every 3 seconds
    if (Counter % 3 == 0 && !performanceMode)
        Stca->OnRefresh(this);

    // Every 30 seconds poll OCL data
    if (Counter % 30 == 0) {
        if (ControllerMyself().IsValid() && ControllerMyself().IsController()) {
            if (SharedData::OCLEnabled) {
                // Don't start a new OCL fetch if the previous one is still in-flight.
                if (RawOCLData.valid() &&
                    RawOCLData.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                    DisplayUserMessage("Message", "CoVCL PlugIn", string("Skipping OCL data refresh, previous query still pending ").c_str(), false, false, false, false, false);
                    return;
                }
                RawOCLData = async(launch::async, &CoVCLPlugIn::LoadOCLData, this);
            }
        }
    }
}

void CoVCLPlugIn::OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area)
{
    if (FunctionId == CoVCLTags::FUNCTION_CONFLICT_POPUP) {
        SetASELAircraft(FlightPlanSelect(sItemString));
        OpenPopupList(Area, "Conflict Group", 1);
        AddPopupListElement(CoVCLCharacters::Moon.c_str(), "", CoVCLTags::FUNCTION_HANDLE_CONFLICT_GROUP);
        AddPopupListElement(CoVCLCharacters::Star.c_str(), "", CoVCLTags::FUNCTION_HANDLE_CONFLICT_GROUP);
        AddPopupListElement(CoVCLCharacters::Losange.c_str(), "", CoVCLTags::FUNCTION_HANDLE_CONFLICT_GROUP);
        AddPopupListElement("Remove", "", CoVCLTags::FUNCTION_HANDLE_CONFLICT_GROUP, false, 2, false, true);
    }

    if (FunctionId == CoVCLTags::FUNCTION_HANDLE_CONFLICT_GROUP) {
        if (ConflictGroups.find(FlightPlanSelectASEL().GetCallsign()) != ConflictGroups.end() && strcmp(sItemString, "Remove") == 0)
            ConflictGroups.erase(FlightPlanSelectASEL().GetCallsign());
        else if (strcmp(sItemString, "Remove") != 0)
            ConflictGroups[FlightPlanSelectASEL().GetCallsign()] = sItemString;
    }

    if (FunctionId == CoVCLTags::FUNCTION_OCL_TP) {
        CFlightPlan fp = FlightPlanSelectASEL();
        auto t = fp.GetCallsign();
        if (!fp.IsValid())
            return;

        SharedData::OCL_Tooltip_timer = std::chrono::system_clock::now();

        if (HasOCL(fp.GetCallsign())) {
            SharedData::OCL_Tooltip_string = GetFullOCL(fp.GetCallsign());
            SharedData::OCL_Tooltip_pt = Pt;
        }
    }
}

void CoVCLPlugIn::LoadConfigFile(bool fromWeb)
{
    DisplayUserMessage("Message", "CoVCL PlugIn", "Reading config file...", false, false, false, false, false);

    try {
        if (fromWeb) {
            httplib::Client cli(CONFIG_ONLINE_URL_BASE);
            if (auto res = cli.Get(CONFIG_ONLINE_URL_PATH)) {
                if (res->status == 200) {
                    std::istringstream is(res->body, std::ios_base::binary | std::ios_base::in);

                    CoVCLConfig = toml::parse(is, "std::string");
                }
                else
                    fromWeb = false;
            }
            else 
                fromWeb = false;
                
            cli.stop();

            if (!fromWeb)
                DisplayUserMessage("Message", "CoVCL PlugIn", "Error loading web config, reverting to local file!", false, false, false, false, false);
                
        } 

        if (!fromWeb) {
            CoVCLConfig = toml::parse(DllPath + "\\CoVCL.toml");
        }
            
        CanLoadRadarScreen = true;
        DisplayUserMessage("Message", "CoVCL PlugIn", "Config file loaded!", false, false, false, false, false);
    }
    catch (const std::exception& exc) {
        CanLoadRadarScreen = false;
        DisplayUserMessage("Message", "CoVCL PlugIn", string("Error reading config file " + string(exc.what())).c_str(), false, false, false, false, false);
    }
}

string CoVCLPlugIn::LoadOCLData()
{
    try {
        httplib::Client cli("https://nattrak.vatsim.net");
        cli.set_connection_timeout(0, 500000);
		cli.set_read_timeout(0, 500000);
		cli.set_write_timeout(0, 500000);

        if (auto res = cli.Get("/api/plugins")) {
            if (res->status == 200) {
                cli.stop();
                return res->body;
            }
            else {
                cli.stop();
                DisplayUserMessage("Message", "CoVCL PlugIn", string("Error fetching OCL data, server returned HTTP code: " + to_string(res->status) + " body: " + res->body).c_str(), false, false, false, false, false);
                return "[]";
            }
        }

        cli.stop();
    }
    catch (const std::exception& exc) {
        DisplayUserMessage("Message", "CoVCL PlugIn", string("Error fetching OCL data: " + string(exc.what())).c_str(), false, false, false, false, false);
        return "[]";
    }

    return "[]";

    //return "[ { \"callsign\": \"BER1PE\", \"status\": \"CLEARED\", \"nat\": \"A\", \"fix\": \"MALOT\", \"level\": \"320\", \"mach\": \"0.89\", \"estimating_time\": \"1921\", \"clearance_issued\": \"2021-03-26 00:21:19\", \"extra_info\": \"CROSS MALOT NOT BEFORE 1925\" }, { \"callsign\":\"ADB3908\", \"status\":\"PENDING\", \"nat\":\"RR\", \"fix\":\"PORTI\", \"level\": \"350\", \"mach\": \"0.82\", \"estimating_time\":\"18:41\", \"clearance_issued\":null, \"extra_info\":null } ]";
}
