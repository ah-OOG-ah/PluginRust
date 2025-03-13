#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
	#include <windows.h>
#endif
#include <PluginAPI/Plugin.h>
#include <imgui/imgui.h>

#include <cstdio>
#include <cstdlib>
#include <array>
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <picojson/picojson.h>

#ifdef WIN32
# define FEXPORT __declspec(dllexport)
#else
# define FEXPORT 
#endif

namespace rs
{
	class RustPlugin : public ed::IPlugin2
	{
	public:
		bool Init(bool isWeb, int sedVersion) override {
			m_buildLangDefinition();
			m_spv = nullptr;
			
#ifdef _WIN32
			m_codegenPath = "rustc_codegen_spirv.dll";
#else
			m_codegenPath = "librustc_codegen_spirv.so";
#endif	
		
			if (sedVersion == 1003005)
				m_hostVersion = 1;
			else
				m_hostVersion = GetHostIPluginMaxVersion();

			return true;
		}

		void InitUI(void* ctx) override {
			ImGui::SetCurrentContext((ImGuiContext*)ctx);
		}
		void OnEvent(void* e) override { }
		void Update(float delta) override {
			if (m_hostVersion >= 2 && ImGuiFileDialogIsDone("PluginRustCodegenFile")) {
				if (ImGuiFileDialogGetResult()) {
					char tempFilepath[1024] = { 0 };
					ImGuiFileDialogGetPath(tempFilepath);
					m_codegenPath = std::string(tempFilepath);
				}

				ImGuiFileDialogClose("PluginRustCodegenFile");
			}
		}
		void Destroy() override {
			if (m_spv)
				free(m_spv);
		}

		bool IsRequired() override { return false; }
		bool IsVersionCompatible(int version) override { return false; }

		void BeginRender() override { }
		void EndRender() override { }

		void Project_BeginLoad() override { }
		void Project_EndLoad() override { }
		void Project_BeginSave() override { }
		void Project_EndSave() override { }
		bool Project_HasAdditionalData() override { return false; }
		const char* Project_ExportAdditionalData() override { return nullptr; }
		void Project_ImportAdditionalData(const char* xml) override { }
		void Project_CopyFilesOnSave(const char* dir) override { }

		/* list: file, newproject, project, createitem, window, custom */
		bool HasCustomMenuItem() override { return false; }
		bool HasMenuItems(const char* name) override { return false; }
		void ShowMenuItems(const char* name) override { }

		/* list: pipeline, shaderpass_add (owner = ShaderPass), pluginitem_add (owner = char* ItemType, extraData = PluginItemData) objects, editcode (owner = char* ItemName) */
		bool HasContextItems(const char* name) override { return false; }
		void ShowContextItems(const char* name, void* owner = nullptr, void* extraData = nullptr) override { }

		// system variable methods
		int SystemVariables_GetNameCount(ed::plugin::VariableType varType) override { return 0; }
		const char* SystemVariables_GetName(ed::plugin::VariableType varType, int index) override { return nullptr; }
		bool SystemVariables_HasLastFrame(char* name, ed::plugin::VariableType varType) override { return false; }
		void SystemVariables_UpdateValue(char* data, char* name, ed::plugin::VariableType varType, bool isLastFrame) override { }

		// function variables
		int VariableFunctions_GetNameCount(ed::plugin::VariableType vtype) override { return 0; }
		const char* VariableFunctions_GetName(ed::plugin::VariableType varType, int index) override { return nullptr; }
		bool VariableFunctions_ShowArgumentEdit(char* fname, char* args, ed::plugin::VariableType vtype) override { return false; }
		void VariableFunctions_UpdateValue(char* data, char* args, char* fname, ed::plugin::VariableType varType) override { }
		int VariableFunctions_GetArgsSize(char* fname, ed::plugin::VariableType varType) override { return 0; }
		void VariableFunctions_InitArguments(char* args, char* fname, ed::plugin::VariableType vtype) override { }
		const char* VariableFunctions_ExportArguments(char* fname, ed::plugin::VariableType vtype, char* args) override { return nullptr; }
		void VariableFunctions_ImportArguments(char* fname, ed::plugin::VariableType vtype, char* args, const char* argsString) override { }

		// object manager stuff
		bool Object_HasPreview(const char* type) override { return false; }
		void Object_ShowPreview(const char* type, void* data, unsigned int id) override { }
		bool Object_IsBindable(const char* type) override { return false; }
		bool Object_IsBindableUAV(const char* type) override { return false; }
		void Object_Remove(const char* name, const char* type, void* data, unsigned int id) override { }
		bool Object_HasExtendedPreview(const char* type) override { return false; }
		void Object_ShowExtendedPreview(const char* type, void* data, unsigned int id) override { }
		bool Object_HasProperties(const char* type) override { return false; }
		void Object_ShowProperties(const char* type, void* data, unsigned int id) override { }
		void Object_Bind(const char* type, void* data, unsigned int id) override { }
		const char* Object_Export(char* type, void* data, unsigned int id) override { return nullptr; }
		void Object_Import(const char* name, const char* type, const char* argsString) override { }
		bool Object_HasContext(const char* type) override { return false; }
		void Object_ShowContext(const char* type, void* data) override { }

		// pipeline item stuff
		bool PipelineItem_HasProperties(const char* type, void* data) override { return false; }
		void PipelineItem_ShowProperties(const char* type, void* data) override { }
		bool PipelineItem_IsPickable(const char* type, void* data) override { return false; }
		bool PipelineItem_HasShaders(const char* type, void* data) override { return false; }
		void PipelineItem_OpenInEditor(const char* type, void* data) override { }
		bool PipelineItem_CanHaveChild(const char* type, void* data, ed::plugin::PipelineItemType itemType) override { return false; }
		int PipelineItem_GetInputLayoutSize(const char* type, void* data) override { return 0; }
		void PipelineItem_GetInputLayoutItem(const char* type, void* data, int index, ed::plugin::InputLayoutItem& out) override { }
		void PipelineItem_Remove(const char* itemName, const char* type, void* data) override { }
		void PipelineItem_Rename(const char* oldName, const char* newName) override { }
		void PipelineItem_AddChild(const char* owner, const char* name, ed::plugin::PipelineItemType type, void* data) override { }
		bool PipelineItem_CanHaveChildren(const char* type, void* data) override { return false; }
		void* PipelineItem_CopyData(const char* type, void* data) override { return nullptr; }
		void PipelineItem_Execute(void* Owner, ed::plugin::PipelineItemType OwnerType, const char* type, void* data) override { }
		void PipelineItem_Execute(const char* type, void* data, void* children, int count) override { }
		void PipelineItem_GetWorldMatrix(const char* type, void* data, float(&pMat)[16]) override { }
		bool PipelineItem_Intersect(const char* type, void* data, const float* rayOrigin, const float* rayDir, float& hitDist) override { return false; }
		void PipelineItem_GetBoundingBox(const char* type, void* data, float(&minPos)[3], float(&maxPos)[3]) override { }
		bool PipelineItem_HasContext(const char* type, void* data) override { return false; }
		void PipelineItem_ShowContext(const char* type, void* data) override { }
		const char* PipelineItem_Export(const char* type, void* data) override { return nullptr; }
		void* PipelineItem_Import(const char* ownerName, const char* name, const char* type, const char* argsString) override { return nullptr; }
		void PipelineItem_MoveDown(void* ownerData, const char* ownerType, const char* itemName) override { }
		void PipelineItem_MoveUp(void* ownerData, const char* ownerType, const char* itemName) override { }
		void PipelineItem_ApplyGizmoTransform(const char* type, void* data, float* transl, float* scale, float* rota) override { }
		void PipelineItem_GetTransform(const char* type, void* data, float* transl, float* scale, float* rota) override { }
		void PipelineItem_DebugVertexExecute(void* Owner, ed::plugin::PipelineItemType OwnerType, const char* type, void* data, unsigned int colorVarLoc) override { }
		int PipelineItem_DebugVertexExecute(const char* type, void* data, const char* childName, float rx, float ry, int vertexGroup) override { return 0; }
		void PipelineItem_DebugInstanceExecute(void* Owner, ed::plugin::PipelineItemType OwnerType, const char* type, void* data, unsigned int colorVarLoc) override { }
		int PipelineItem_DebugInstanceExecute(const char* type, void* data, const char* childName, float rx, float ry, int vertexGroup) override { return 0; }
		unsigned int PipelineItem_GetVBO(const char* type, void* data) override { return 0; }
		unsigned int PipelineItem_GetVBOStride(const char* type, void* data) override { return 0; }
		bool PipelineItem_CanChangeVariables(const char* type, void* data) override { return false; }
		bool PipelineItem_IsDebuggable(const char* type, void* data) override { return false; }
		bool PipelineItem_IsStageDebuggable(const char* type, void* data, ed::plugin::ShaderStage stage) override { return false; }
		void PipelineItem_DebugExecute(const char* type, void* data, void* children, int count, int* debugID) override { }
		unsigned int PipelineItem_GetTopology(const char* type, void* data) override { return 0; }
		unsigned int PipelineItem_GetVariableCount(const char* type, void* data) override { return 0; }
		const char* PipelineItem_GetVariableName(const char* type, void* data, unsigned int variable) override { return nullptr; }
		ed::plugin::VariableType PipelineItem_GetVariableType(const char* type, void* data, unsigned int variable) override { return ed::plugin::VariableType::Float1; }
		float PipelineItem_GetVariableValueFloat(const char* type, void* data, unsigned int variable, int col, int row) override { return 0; }
		int PipelineItem_GetVariableValueInteger(const char* type, void* data, unsigned int variable, int col) override { return 0; }
		bool PipelineItem_GetVariableValueBoolean(const char* type, void* data, unsigned int variable, int col) override { return false; }
		unsigned int PipelineItem_GetSPIRVSize(const char* type, void* data, ed::plugin::ShaderStage stage) override { return 0; }
		unsigned int* PipelineItem_GetSPIRV(const char* type, void* data, ed::plugin::ShaderStage stage) override { return nullptr; }
		void PipelineItem_DebugPrepareVariables(const char* type, void* data, const char* name) override { }
		bool PipelineItem_DebugUsesCustomTextures(const char* type, void* data) override { return false; }
		unsigned int PipelineItem_DebugGetTexture(const char* type, void* data, int loc, const char* variableName) override { return 0; }
		void PipelineItem_DebugGetTextureSize(const char* type, void* data, int loc, const char* variableName, int& x, int& y, int& z) override { }

		// options
		bool Options_HasSection() override { return true; }
		void Options_RenderSection() override {
			ImGui::Text("Codegen file: %s", m_codegenPath.c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 180);
			if (ImGui::Button("CHANGE", ImVec2(-1, 0))) {
				if (m_hostVersion >= 2)
					ImGuiFileDialogOpen("PluginRustCodegenFile", "Select the codegen file", "DLL file (*.dll;*.so){.dll,.so},.*");
			}
		}
		void Options_Parse(const char* key, const char* val) override
		{
			if (strcmp(key, "codegen") == 0) {
				m_codegenPath = std::string(val);
				if (!std::filesystem::exists(m_codegenPath)) {
					printf("%s\n[RSHADERS] codegen .dll doesn't exists.\n", m_codegenPath.c_str());
				}
			}
		}
		int Options_GetCount() override { return 1; } // path to codegen .dll
		const char* Options_GetKey(int index) override
		{
			if (index == 0)
				return "codegen";
			return nullptr;
		}
		const char* Options_GetValue(int index) override
		{
			if (index == 0)
				return m_codegenPath.c_str();
			return nullptr;
		}

		// languages
		int CustomLanguage_GetCount() override { return 1; }
		const char* CustomLanguage_GetName(int langID) override { return "Rust"; }
		const unsigned int* CustomLanguage_CompileToSPIRV(
                int langID,
                const char* src,
                size_t src_len,
                ed::plugin::ShaderStage stage,
                const char* entry,
                ed::plugin::ShaderMacro* macros,
                size_t macroCount,
                size_t* spv_length,
                bool* compiled) override {
			// write to lib.rs file
			std::ofstream writer("rust_box/rss/src/lib.rs");
			writer << src;
			writer.close();

			// run cargo build
			std::string output = m_exec("cd rust_box; cargo build --message-format=json --release");

			if (output.empty()) {
				*compiled = false;
				AddMessage(Messages, ed::plugin::MessageType::Error, nullptr, "Cargo failed to run - no output", -1);
				return m_spv;
			}
			
			printf("starting to parse!\n");
			
			// TODO: split by \n then parse...
			bool compileSuccess = true;
			std::stringstream outputParser(output);
			std::string outputSegment;
			while (std::getline(outputParser, outputSegment)) {
				picojson::value v;
				std::string err = picojson::parse(v, outputSegment);

				if (!err.empty()) {
					AddMessage(
                        Messages,
                        ed::plugin::MessageType::Error,
                        GetMessagesCurrentItem(Messages),
                        "Cargo failed to run - errors during compilation", -1);
					*compiled = false;
					return m_spv;
				}

				if (v.is<picojson::object>()) {

					picojson::object& obj = v.get<picojson::object>();
					std::string reason = obj["reason"].get<std::string>();
					if (reason == "compiler-message") {
						picojson::object& msg = obj["message"].get<picojson::object>();

						std::string level = msg["level"].get<std::string>();
						std::string message = msg["message"].get<std::string>();

						ed::plugin::MessageType mtype = ed::plugin::MessageType::Message;
						if (level == "error")
							mtype = ed::plugin::MessageType::Error;
						else if (level == "warning")
							mtype = ed::plugin::MessageType::Warning;

						int line = -1;
						picojson::array& spans = msg["spans"].get<picojson::array>();
						for (auto& span : spans) {
							picojson::object& spanObj = span.get<picojson::object>();
							int tempLine = (int)spanObj["line_start"].get<double>();

							if (tempLine == line)
								continue;
							line = tempLine;


							AddMessage(Messages, mtype, GetMessagesCurrentItem(Messages), message.c_str(), line);
						}
						
						if (level == "error") 
							compileSuccess = false;
                    } else if (reason == "build-finished") {
                        bool status = obj["success"].get<bool>();
                        if (!status)
                            compileSuccess = false;
                    }
				}
			}

			if (!compileSuccess) {
				*compiled = false;
				return nullptr;
			}

			printf("starting to read spv!\n");
			

			// read from .spv file
			FILE* reader = fopen("rust_box/spirv/shader.spv", "rb");
			if (reader == nullptr) {
				*compiled = false;
				return nullptr;
			}
			fseek(reader, 0, SEEK_END);
			long fsize = ftell(reader);
			fseek(reader, 0, SEEK_SET);
			if (m_spv != nullptr)
				free(m_spv);
			m_spv = (unsigned int*)malloc((fsize/4 + 1)*4);
			fread(m_spv, 1, fsize, reader);
			fclose(reader);

			*spv_length = fsize / 4;
			*compiled = true;

			return m_spv;
		}

		const char* CustomLanguage_ProcessGeneratedGLSL(int langID, const char* srcPtr) override
		{
			return srcPtr;
		}
		bool CustomLanguage_SupportsAutoUniforms(int langID) override { return true; }
		bool CustomLanguage_IsDebuggable(int langID) override { return true; }
		const char* CustomLanguage_GetDefaultExtension(int langID) override { return "rs"; }

		// language text editor
		bool ShaderEditor_Supports(int langID) override { return false; }
		void ShaderEditor_Open(int langID, int editorID, const char* data, int dataLen) override { }
		void ShaderEditor_Render(int langID, int editorID) override { }
		void ShaderEditor_Close(int langID, int editorID) override { }
		const char* ShaderEditor_GetContent(int langID, int editorID, size_t* dataLength) override { return nullptr; }
		bool ShaderEditor_IsChanged(int langID, int editorID) override { return false; }
		void ShaderEditor_ResetChangeState(int langID, int editorID) override { }
		bool ShaderEditor_CanUndo(int langID, int editorID) override { return false; }
		bool ShaderEditor_CanRedo(int langID, int editorID) override { return false; }
		void ShaderEditor_Undo(int langID, int editorID) override { }
		void ShaderEditor_Redo(int langID, int editorID) override { }
		void ShaderEditor_Cut(int langID, int editorID) override { }
		void ShaderEditor_Paste(int langID, int editorID) override { }
		void ShaderEditor_Copy(int langID, int editorID) override { }
		void ShaderEditor_SelectAll(int langID, int editorID) override { }
		bool ShaderEditor_HasStats(int langID, int editorID) override { return false; }

		// code editor
		void CodeEditor_SaveItem(const char* src, int srcLen, const char* id) override {}
		void CodeEditor_CloseItem(const char* id) override {}
		bool LanguageDefinition_Exists(int id) override { return true; }
		int LanguageDefinition_GetKeywordCount(int id) override {
			return (int) m_langDefKeywords.size();
		}

		const char** LanguageDefinition_GetKeywords(int id) override {
			return m_langDefKeywords.data();
		}

		int LanguageDefinition_GetTokenRegexCount(int id) override {
			return (int) m_langDefRegex.size();
		}

		const char* LanguageDefinition_GetTokenRegex(int index, ed::plugin::TextEditorPaletteIndex& palIndex, int id) override {
            palIndex = m_langDefRegex[index].second;
            return m_langDefRegex[index].first;
        }

		int LanguageDefinition_GetIdentifierCount(int id) override {
			return (int) m_langDefIdentifiers.size();
		}

		const char* LanguageDefinition_GetIdentifier(int index, int id) override
		{
			return m_langDefIdentifiers[index].first;
		}
		const char* LanguageDefinition_GetIdentifierDesc(int index, int id) override
		{
			return m_langDefIdentifiers[index].second;
		}
		const char* LanguageDefinition_GetCommentStart(int id) override
		{
			return "/*";
		}
		const char* LanguageDefinition_GetCommentEnd(int id) override
		{
			return "*/";
		}
		const char* LanguageDefinition_GetLineComment(int id) override
		{
			return "//";
		}
		bool LanguageDefinition_IsCaseSensitive(int id) override { return true; }
		bool LanguageDefinition_GetAutoIndent(int id) override { return true; }
		const char* LanguageDefinition_GetName(int id) override { return "Rust"; }
		const char* LanguageDefinition_GetNameAbbreviation(int id) override { return "RS"; }

		// autocomplete
		int Autocomplete_GetCount(ed::plugin::ShaderStage stage) override { return 0; }
		const char* Autocomplete_GetDisplayString(ed::plugin::ShaderStage stage, int index) override { return nullptr; }
		const char* Autocomplete_GetSearchString(ed::plugin::ShaderStage stage, int index) override { return nullptr; }
		const char* Autocomplete_GetValue(ed::plugin::ShaderStage stage, int index) override { return nullptr; }

		// file change checks
		int ShaderFilePath_GetCount() override { return 0; }
		const char* ShaderFilePath_Get(int index) override { return nullptr; }
		bool ShaderFilePath_HasChanged() override { return false; }
		void ShaderFilePath_Update() override {}

		// misc
		bool HandleDropFile(const char* filename) override { return false; }
		void HandleRecompile(const char* itemName) override {}
		void HandleRecompileFromSource(const char* itemName, int sid, const char* shaderCode, int shaderSize) override {}
		void HandleShortcut(const char* name) override {}
		void HandlePluginMessage(const char* sender, char* msg, int msgLen) override {}
		void HandleApplicationEvent(ed::plugin::ApplicationEvent event, void* data1, void* data2) override {}
		void HandleNotification(int id) override {}

		// IPlugin2 stuff
		bool PipelineItem_SupportsImmediateMode(const char* type, void* data, ed::plugin::ShaderStage stage) override { return false; }
		bool PipelineItem_HasCustomImmediateModeCompiler(const char* type, void* data, ed::plugin::ShaderStage stage) override { return false; }
		bool PipelineItem_ImmediateModeCompile(const char* type, void* data, ed::plugin::ShaderStage stage, const char* expression) override { return false; }

		unsigned int ImmediateMode_GetSPIRVSize() override { return 0; }
		unsigned int* ImmediateMode_GetSPIRV() override { return nullptr; }
		unsigned int ImmediateMode_GetVariableCount() override { return 0; }
		const char* ImmediateMode_GetVariableName(unsigned int index) override { return nullptr; }
		int ImmediateMode_GetResultID() override { return 0; }

	private:
		int m_hostVersion;

		std::vector<const char*> m_langDefKeywords;
		std::vector<std::pair<const char*, ed::plugin::TextEditorPaletteIndex>> m_langDefRegex;
		std::vector<std::pair<const char*, const char*>> m_langDefIdentifiers;
		void m_buildLangDefinition()
		{
			// keywords
			m_langDefKeywords.clear();
			m_langDefKeywords = {
				"as", "break", "const", "continue",
				"crate", "else", "enum", "extern", "false",
				"fn", "rust-gpufor", "if", "impl",
				"in", "let", "loop", "match", "mod",
				"move", "mut", "pub", "ref",
				"return", "self", "Self", "static",
				"struct", "super", "trait", "true",
				"type", "unsafe", "use", "where",
				"while", "f32", "Output", "Input", "UniformConstant",
				"Vec2", "Vec3", "Vec4",
				"Mat2", "Mat3", "Mat4", "new"
			};

			// regex
			m_langDefRegex.clear();
			m_langDefRegex.emplace_back(R"([ \t]*#\!?\[[^\]]*\])", ed::plugin::TextEditorPaletteIndex::Preprocessor);
			m_langDefRegex.emplace_back(R"(L?\"(\\.|[^\"])*\")", ed::plugin::TextEditorPaletteIndex::String);
			m_langDefRegex.emplace_back(R"(\'\\?[^\']\')", ed::plugin::TextEditorPaletteIndex::CharLiteral);
			m_langDefRegex.emplace_back("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ed::plugin::TextEditorPaletteIndex::Number);
			m_langDefRegex.emplace_back("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ed::plugin::TextEditorPaletteIndex::Number);
			m_langDefRegex.emplace_back("0[0-7]+[Uu]?[lL]?[lL]?", ed::plugin::TextEditorPaletteIndex::Number);
			m_langDefRegex.emplace_back("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ed::plugin::TextEditorPaletteIndex::Number);
			m_langDefRegex.emplace_back("[a-zA-Z_][a-zA-Z0-9_]*", ed::plugin::TextEditorPaletteIndex::Identifier);
			m_langDefRegex.emplace_back(R"([\[\]\{\}\!\%\^\&\*\(\)\-\+\=\~\|\<\>\?\/\;\,\.])", ed::plugin::TextEditorPaletteIndex::Punctuation);

			// identifiers
			m_langDefIdentifiers.clear();
		}

		std::string m_codegenPath;
		void m_updateEnv() {
			std::string actualPath = m_codegenPath;
			if (std::filesystem::path(m_codegenPath).is_relative())
				actualPath = (std::filesystem::current_path() / std::filesystem::path(m_codegenPath)).string();

			std::string flags = "-Z codegen-backend=" + actualPath + " -C target-feature=+glsl450";

#if defined(_WIN32)
			_putenv_s("RUSTFLAGS", flags.c_str());
#else
			setenv("RUSTFLAGS", flags.c_str(), 1);
#endif

		}
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"
		static std::string m_exec(const char* cmd) {
#pragma clang diagnostic pop
#if defined(_WIN32)
			std::array<char, 128> buffer;
			std::string result;
			std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
			if (!pipe) {
				printf("Failed to run exec()");
				return "";
			}
			while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
				result += buffer.data();
			}
			return result;
#else
			char buffer[128];
			std::string result;
			std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
			if (!pipe) {
				printf("Failed to run exec()");
				return "";
			}
			while (fgets(buffer, 128, pipe.get()) != nullptr) {
				result += buffer;
			}
			return result;
#endif
		}


		unsigned int* m_spv;
	};
}

extern "C" {
	FEXPORT rs::RustPlugin* CreatePlugin() {
		return new rs::RustPlugin();
	}
	FEXPORT void DestroyPlugin(rs::RustPlugin* ptr) {
		delete ptr;
	}
	FEXPORT int GetPluginAPIVersion() {
		return 3;
	}
	FEXPORT int GetPluginVersion() {
		return 1;
	}
	FEXPORT const char* GetPluginName() {
		return "Rust";
	}
}

#ifdef _WIN32
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif
