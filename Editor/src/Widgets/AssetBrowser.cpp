#include "Widgets/AssetBrowser.h"
#include "Core/ResourceManager.h"
#include "IconsFontAwesome5.h"
#include "Core/FileSystem.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "EditorLayer.h"
#include "Scene/SceneManager.h"

static SharedPtr<Texture2D> MapFileToImage(std::string filename)
{
    size_t dot = filename.find_last_of(".");
    std::string ext;
    if (dot != std::string::npos)
    {
        ext = filename.substr(dot, filename.size() - dot);
    }

    if (ext == ".jpg"
     || ext == ".png"
    )
        return ResourceManager::GetTexture2D(filename);
    else if (ext == ".cf")
        return ResourceManager::GetTexture2D(ASSETS + "/EngineAssets/Icons/campfire.png");
    else if (ext == ".lua")
        return ResourceManager::GetTexture2D(ASSETS + "/EngineAssets/Icons/lua.png");
    else // Generic file image
        return ResourceManager::GetTexture2D(ASSETS + "/EngineAssets/Icons/file.png");

}

AssetBrowser::AssetBrowser(EditorLayer* editor)
    : mEditor(editor)
{
    // TODO save the last visited path into some engine meta file
    // so that browser opens back where it left off
    currPath = ASSETS;
}

void AssetBrowser::OpenFile(const std::filesystem::path& path)
{
    std::string filepath = path.string();
    size_t dot = filepath.find_last_of(".");
    std::string ext;
    if (dot == std::string::npos)
    {
        return;
    }

    ext = filepath.substr(dot, filepath.size() - dot);

    // Scene files
    if (ext == ".cf")
    {
        mEditor->editorScene = mEditor->activeScene = SceneManager::LoadScene(filepath);
    }
    // All other files, use the default program set up for that extension
    else
    {
        FileSystem::OpenFileWithDefaultProgram(path.string().c_str());
    }
}

void AssetBrowser::RecurseCurrentDir(const std::filesystem::path& path)
{
    // Stop when we traverse beyond the Assets dir
    if (path == std::filesystem::path(ASSETS).parent_path())
    {
        return;
    }

    RecurseCurrentDir(path.parent_path());

    if (std::filesystem::is_directory(path))
    {
        if (ImGui::Button(path.filename().string().c_str()))
        {
            currPath = path;
        }
        ImGui::SameLine(); ImGui::Text(">"); ImGui::SameLine();
    }
}

void AssetBrowser::OnImGuiRender(bool* isOpen)
{
    static float scale = 3.0f;
    static bool isList = false;
    uint32_t size = 40;

    // Left column -- displays directory list
    ImGui::Begin("Assets", isOpen);

    float contentWidth = ImGui::GetContentRegionAvail().x;
    float contentHeight = ImGui::GetContentRegionAvail().y;

    // Left column -- displays tree of dirs
    ImGui::Columns(2, "assetColumns");
    ImGui::SetColumnWidth(0, contentWidth * 0.3f);
    ImGui::BeginChild("Directory", ImGui::GetContentRegionAvail(), true);
    {
        RecursivelyDisplayDirectories(std::filesystem::path(ASSETS));
    }
    ImGui::EndChild();
    ImGui::NextColumn();
    ImGui::SameLine();

    // Right column -- displays contents of selected directory
    ImGui::BeginChild("Content", ImGui::GetContentRegionAvail(), true);
    {
        // Display list of dirs from Assets to the currently selected path
        // Recurse since its cleaner to print in reverse order using parent_path
        // instead of getting subdirs by parsing the string path
        RecurseCurrentDir(currPath);

        ImGui::Separator();

        ImGui::Checkbox("List View", &isList);
        ImGui::SameLine();
        // Scaling size for buttons
        ImGui::SliderFloat("Scale", &scale, 1.0f, 3.0f);
        ImGui::Separator();
        ImVec2 buttonSize(size * scale, size * scale);

        ImGuiStyle& style = ImGui::GetStyle();
        float windowVisibleX2 = ImGui::GetWindowPos().x + contentWidth;

        int buttonCount = 0;
        for (auto& p : std::filesystem::directory_iterator(currPath))
        {
            buttonCount++;
        }

        if (!isList)
        {
            int numColumns = ImGui::GetContentRegionAvail().x / buttonSize.x;
            numColumns = numColumns > 0 ? numColumns : 1;
            ImGui::Columns(numColumns, nullptr, false);
        }

        int n = 0;
        for (auto& p : std::filesystem::directory_iterator(currPath))
        {
            ImGui::PushID(n);
            std::string filename = p.path().filename().string();

            //if (ImGui::Selectable(filename.c_str(), n == mSelectedItem))
            //{
            //    mSelectedItem = n;
            //}
            //ImGui::SameLine();

            if (isList) // tabulated
            {
                std::string icon = std::filesystem::is_directory(p.path())
                    ? ICON_FA_FOLDER
                    : ICON_FA_FILE
                ;
                ImGui::Text(icon.c_str());
                ImGui::SameLine();
                if (ImGui::Button(filename.c_str()))
                {
                    if (std::filesystem::is_directory(p.path()))
                    {
                        currPath = std::filesystem::relative(p.path());
                    }
                    else
                    {
                        // TODO display in assetViewer
                    }
                }
            }
            else // display icons
            {
                ImGui::BeginGroup();

                SharedPtr<Texture2D> icon;

                bool isDir = std::filesystem::is_directory(p.path());
                icon = isDir
                    ? ResourceManager::GetTexture2D(ASSETS + "/EngineAssets/Icons/folder.png")
                    : MapFileToImage(p.path().string())
                ;

                if (ImGui::ImageButton((ImTextureID)icon->GetRenderID(), buttonSize, ImVec2(0, 1), ImVec2(1, 0)))
                {
                    // Display in inspector
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    isDir
                        ? currPath = std::filesystem::relative(p.path())
                        : OpenFile(p.path())
                    ;
                }

                if (ImGui::BeginPopupContextItem("Right Click Menu"))
                {
                    if (isDir)
                    {
                        if (ImGui::Button("Open Directory"))
                        {

                            FileSystem::OpenInDirectory(p.path().string().c_str());
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                    else
                    {
                        if (ImGui::Button("Browse"))
                        {
                            FileSystem::OpenFileWithDefaultProgram(p.path().string().c_str());
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::OpenPopupOnItemClick("Right Click Menu", 1);

                ImGui::TextWrapped(filename.c_str());
            }

            if (!isList)
            {
                ImGui::EndGroup();
                ImGui::NextColumn();
            }
            ImGui::PopID();

            n++;
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void AssetBrowser::RecursivelyDisplayDirectories(std::filesystem::path dirPath)
{
    static ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

    //static int selection_mask = (1 << 2); // Dumb representation of what may be user-side selection state. You may carry selection state inside or outside your objects in whatever format you see fit.
    //const bool is_selected = (selection_mask & (1 << i)) != 0;
    //if (is_selected)
    //    flags |= ImGuiTreeNodeFlags_Selected;

    if (!std::filesystem::is_directory(dirPath))
    {
        return;
    }

    ImGui::Text(ICON_FA_FOLDER); ImGui::SameLine();
    // Not sure why this isnt working outside the recursive function
    if (dirPath.string() == ASSETS) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    }
    bool nodeOpen = ImGui::TreeNodeEx(dirPath.filename().string().c_str(), flags);
    // Update content view ImGuiwith selected directory
    //ImGui::Selectable()

    if (ImGui::IsItemClicked()
        && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x)
        > ImGui::GetTreeNodeToLabelSpacing()
    ){
        currPath = std::filesystem::relative(dirPath);
    }

    if (nodeOpen)
    {
        for (auto& p : std::filesystem::directory_iterator(dirPath))
        {
            RecursivelyDisplayDirectories(std::filesystem::relative(p.path()));
        }
        ImGui::TreePop();
    }
}
