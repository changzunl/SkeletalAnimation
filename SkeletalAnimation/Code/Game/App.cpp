#include "App.hpp"

#include "GameCommon.hpp"
#include "Game.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Window/Window.hpp"

#include <math.h>
#include <vcruntime_string.h>

App* g_theApp = nullptr;                      // Created and owned by Main_Windows.cpp
InputSystem* g_theInput = nullptr;            // Created and owned by the App
Window* g_theWindow = nullptr;                // Created and owned by the App
Renderer* g_theRenderer = nullptr;            // Created and owned by the App
AudioSystem* g_theAudio = nullptr;            // Created and owned by the App
Game* g_theGame = nullptr;                    // Created and owned by the App
BitmapFont* g_testFont = nullptr;             // Created and owned by the App

App::App()
{
}

App::~App() 
{
}

void App::Startup()
{
    PopulateGameConfigBlackboard();


    EventSystemConfig evtConfig = EventSystemConfig();
    g_theEventSystem = new EventSystem(evtConfig);
    InputSystemConfig inputConfig = InputSystemConfig();
    g_theInput = new InputSystem(inputConfig);
    WindowConfig windowConfig = WindowConfig();
    windowConfig.m_windowTitle = g_gameConfigBlackboard.GetValue("windowTitle", "Protogame2D");
    windowConfig.m_clientAspect = g_gameConfigBlackboard.GetValue("windowAspect", 2.0f);
    windowConfig.m_inputSystem = g_theInput;
    g_theWindow = new Window(windowConfig);
    RendererConfig rendererConfig = RendererConfig();
    rendererConfig.m_window = g_theWindow;
    g_theRenderer = new Renderer(rendererConfig);
    AudioSystemConfig audioConfig = AudioSystemConfig();
    g_theAudio = new AudioSystem(audioConfig);

    g_theEventSystem->Startup();
    g_theInput->Startup();
    g_theWindow->Startup();
    g_theRenderer->Startup();
    g_theAudio->Startup();

    DebugRenderSystemShutdown();
    DebugRenderConfig config;
    config.m_renderer = g_theRenderer;
    config.m_messagePerScreen = 45;
    DebugRenderSystemStartup(config);

    g_testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

    DevConsoleConfig consoleConfig = DevConsoleConfig();
    consoleConfig.m_renderer = g_theRenderer;
    consoleConfig.m_font = g_testFont;
    consoleConfig.m_linesPerScreen = g_gameConfigBlackboard.GetValue("consoleLines", 20);
    g_theConsole = new DevConsole(consoleConfig);
    g_theConsole->Startup();

    g_theGame = new Game();
    g_theGame->Initialize();
}

void App::RunMainLoop()
{
    // Program main loop; keep running frames until it's time to quit
    while (!IsQuitting())
    {
        RunFrame();
    }
}

void App::Shutdown()
{
    g_theGame->Shutdown();
    delete g_theGame;
    g_theGame = nullptr;

    g_theConsole->Shutdown();
    delete g_theConsole;
    g_theConsole = nullptr;

    g_theAudio->Shutdown();
    g_theRenderer->Shutdown();
    g_theWindow->Shutdown();
    g_theInput->Shutdown();
    g_theEventSystem->Shutdown();
    delete g_theAudio;
    g_theAudio = nullptr;
    delete g_theRenderer;
    g_theRenderer = nullptr;
    delete g_theWindow;
    g_theWindow = nullptr;
    delete g_theInput;
    g_theInput = nullptr;
    delete g_theEventSystem;
    g_theEventSystem = nullptr;
}

void App::RunFrame()
{
    BeginFrame();

    Update();
    Render();
    
    // general inner functions
    HandleGeneralInputEvents();

    EndFrame();

    // handle hard reset
    if (IsRebooting())
    {
        g_theGame->Shutdown();
        delete g_theGame;
        g_theGame = new Game();
        g_theGame->Initialize();
        m_isRebooting = false;
    }
}

void App::Quit()
{
    m_isQuitting = true;
}

void App::HandleGeneralInputEvents()
{
    // time dilation
// 	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
// 	{
//         Clock::GetSystemClock().SetTimeDilation(Clock::GetSystemClock().GetTimeDilation() * 2.0f);
// 		return;
// 	}
// 	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
// 	{
// 		Clock::GetSystemClock().SetTimeDilation(Clock::GetSystemClock().GetTimeDilation() * 0.5f);
// 		return;
// 	}

    // handle reset
    if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
    {
        m_isRebooting = true;
        return;
    }
    if (g_theInput->WasQuitJustRequested())
    {
        Quit();
        return;
    }
}

void App::BeginFrame()
{
    g_theEventSystem->BeginFrame();
    g_theInput->BeginFrame();
    g_theWindow->BeginFrame();
    g_theRenderer->BeginFrame();
    g_theAudio->BeginFrame();
    g_theConsole->BeginFrame();

    Clock::GetSystemClock().SystemBeginFrame();

	DebugAddMessage(Stringf("FPS: %.2f", 1.0 / Clock::GetSystemClock().GetDeltaTime()).c_str(), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}

void App::Update()
{
    g_theGame->Update();
}

void App::Render() const
{
    g_theGame->Render();

    Camera screenCamera;
    screenCamera.SetOrthoView(g_theRenderer->GetViewport().m_mins, g_theRenderer->GetViewport().m_maxs);
    g_theRenderer->BeginCamera(screenCamera);

	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMask(false);
	g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
	g_theRenderer->SetCullMode(CullMode::NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);
	g_theRenderer->BindTexture(nullptr);

    g_theConsole->Render(g_theRenderer->GetViewport());
    g_theRenderer->EndCamera(screenCamera);
}

void App::EndFrame()
{
    g_theEventSystem->EndFrame();
    g_theInput->EndFrame();
    g_theWindow->EndFrame();
    g_theRenderer->EndFrame();
    g_theAudio->EndFrame();
    g_theConsole->EndFrame();
}

void App::PopulateGameConfigBlackboard()
{
    XmlDocument document;
    document.LoadFile("Data/GameConfig.xml");
    g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*document.RootElement());
}

