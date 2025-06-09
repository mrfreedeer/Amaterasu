#pragma once

#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include <filesystem>

#include <string>
#include <vector>
#include <mutex>

class Camera;
class Renderer;
class CommandList;
class DescriptorHeap;
class BitmapFont;
class RemoteConsole;

struct PipelineState;

struct DevConsoleLine {

	DevConsoleLine(Rgba8 const& color, std::string const& text, int frameNumber);

	Rgba8 m_color = Rgba8::WHITE;
	std::string m_text = "";
	int m_frameNumber = 0;
};

enum class DevConsoleMode {
	HIDDEN,
	SHOWN
};

struct DevConsoleConfig {
	DevConsoleMode m_mode = DevConsoleMode::HIDDEN;
	CommandList* m_cmdList = nullptr;
	Renderer* m_renderer = nullptr;
	std::string m_font = "";
	float m_fontAspect = 0.6f;
	float m_maxLinesShown = 20.5f;
	int m_maxCommandHistory = 128;
};

namespace tinyxml2 {
	class XMLElement;
}
typedef tinyxml2::XMLElement XMLElement;

namespace std::filesystem {
	class path;
}

class DevConsole {
public:
	DevConsole(DevConsoleConfig const& config);
	~DevConsole();
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	bool Execute(std::string const& consoleCommandText);
	bool EventExecuteXMLFile(EventArgs& args);
	bool ExecuteXmlCommandScriptNode(XMLElement const& cmdScriptXmlElement);
	bool ExecuteXmlCommandScriptFile(std::filesystem::path const& filePath);
	void AddLine(Rgba8 const& color, std::string const& text);
	void Render(AABB2 const& bounds, CommandList* cmdListOverride = nullptr) const;

	DevConsoleMode GetMode() const;
	void SetMode(DevConsoleMode newMode);
	void SetCommandList(CommandList* cmdList);
	void SetRenderer(Renderer* renderer);
	void SetPSO(PipelineState* pso);
	void SetCamera(Camera const& camera);
	void ToggleMode(DevConsoleMode mode);

	void Clear();

	static Rgba8 const ERROR_COLOR;
	static Rgba8 const WARNING_COLOR;
	static Rgba8 const INFO_MAJOR_COLOR;
	static Rgba8 const INFO_MINOR_COLOR;

	static bool Command_Test(EventArgs& args);
	static bool Event_KeyPressed(EventArgs& args);
	static bool Event_CharInput(EventArgs& args);
	static bool Command_Clear(EventArgs& args);
	static bool Command_Help(EventArgs& args);
	static bool Command_Paste_Text(EventArgs& args);

	Clock m_clock;
	RemoteConsole* m_remoteConsole = nullptr;

protected:
	void Render_OpenFull(AABB2 const& bounds, CommandList& cmdList, BitmapFont& font, float fontAspect = 1.0f) const;
	void Render_InputCaret(CommandList& cmdList, BitmapFont& font, float fontAspect, float cellHeight) const;
	void Render_UserInput(CommandList& cmdList, BitmapFont& font, float fontAspect, float cellHeight) const;

	std::vector<std::string> ProcessCommandLine(std::string const& commandLine) const;
protected:
	DevConsoleConfig m_config;
	DevConsoleMode m_mode = DevConsoleMode::HIDDEN;

	mutable std::mutex m_linesMutex;
	std::vector<DevConsoleLine> m_lines;
	int m_frameNumber = 0;

	Stopwatch m_caretStopwatch;
	std::string m_inputText;
	int m_caretPosition = 0;
	bool m_caretVisible = true;
	int m_maxCommandHistory = 128;
	std::vector<std::string> m_commandHistory;
	int	m_historyIndex = -1;
	int m_scrollingIndex = -1;

	// Heap for CBV and SRV
	DescriptorHeap* m_rscHeap = nullptr;
	// Sampler heap
	DescriptorHeap* m_samplerHeap = nullptr;
	Camera const* m_usedCamera = nullptr;

	PipelineState* m_pso = nullptr;
};