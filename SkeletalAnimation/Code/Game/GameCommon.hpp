#pragma once

#define UNUSED(x) (void)(x);

struct Rgba8;
struct Vec2;
struct Vertex_PCU;

constexpr int DIFFICULTY_EASY      = 1;
constexpr int DIFFICULTY_NORMAL    = 2;
constexpr int DIFFICULTY_HARD      = 3;

class App;
class InputSystem;
class Window;
class Renderer;
class AudioSystem;
class Game;
class BitmapFont;
class NetworkManagerClient;
class NetworkManagerServer;

extern App* g_theApp;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern Renderer* g_theRenderer;
extern AudioSystem* g_theAudio;
extern Game* g_theGame;
extern BitmapFont* g_testFont;

extern NetworkManagerClient* NET_CLIENT;
extern NetworkManagerServer* NET_SERVER;

enum class BillboardType
{
	NONE,
	FACING,
	ALIGNED,
};

const char* GetNameFromType(BillboardType type);
BillboardType GetTypeByName(const char* name, BillboardType defaultType);

class XboxController;

struct PlayerJoin
{
public:
	PlayerJoin() {};
	PlayerJoin(int order) : m_order(order) {};

public:
	int  m_order = 0;
	bool m_joined = false;
	bool m_isKeyboard = false;
	const XboxController* m_controller = nullptr;
};

