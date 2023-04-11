#include "PCH.h"
#include "DXRenderWindow.h"
#include "EditorProject.h"
#include "EditorRenderer.h"
#include "EditorTextureImporter.h"
#include "EditorTextureResources.h"
#include "EditorTextureViewer.h"
#include "qlayout.h"
#include <stb_image.h>

EditorTextureImporter::EditorTextureImporter(EditorProject& IN_proj, EditorRenderer& IN_renderer) :
	project		{ IN_proj		},
	renderer	{ IN_renderer	}
{
}

struct TextureImporterDialog
{
	TextureImporterDialog(EditorRenderer& renderer, void* _ptr, FlexKit::uint2 wh, uint8_t channelCount, FlexKit::RenderSystem& renderSystem)
	{
		size_t rowPitch		= FlexKit::AlignedSize(wh[0] * 4);
		size_t bufferSize	= rowPitch * wh[1];

		converted = FlexKit::TextureBuffer{ wh, 4, bufferSize , FlexKit::SystemAllocator };

		//FlexKit::TextureBuffer buffer							{ wh, 4, bufferSize , FlexKit::SystemAllocator };
		FlexKit::TextureBuffer buffer							{ wh, (byte*)_ptr, 3 };
		FlexKit::TextureBufferView<FlexKit::RGB>	inputView	{ buffer };
		FlexKit::TextureBufferView<FlexKit::RGBA>	outputView	{ converted, rowPitch };

		memset(converted.Buffer, 0, converted.BufferSize());

		for (size_t y_itr = 0; y_itr < wh[1]; y_itr++)
		{
			for (size_t x_itr = 0; x_itr < wh[0]; x_itr++)
			{
				FlexKit::uint2 px = { x_itr, y_itr };
				outputView[px].Red		= inputView[px].Red;
				outputView[px].Green	= inputView[px].Green;
				outputView[px].Blue		= inputView[px].Blue;
			}
		}

		auto textureHandle = FlexKit::LoadTexture(&converted, renderSystem.GetImmediateCopyQueue(), renderSystem, FlexKit::SystemAllocator);

		viewer = new TextureViewer{ renderer, nullptr, textureHandle };
		auto layout = new QBoxLayout(QBoxLayout::Down);

		UI.setupUi(dialog);
		UI.previewWidget->setLayout(layout);
		layout->addWidget(viewer);

		dialog->show();
	}

	float*					imageBuffer		= nullptr;
	std::string				neededFormat;
	QDialog*				dialog			= new QDialog{};
	TextureViewer*			viewer			= nullptr;
	FlexKit::ResourceHandle	texture			= FlexKit::InvalidHandle;
	Ui_TextureImportDialog	UI;
	FlexKit::TextureBuffer	converted;// { {4096, 4096}, 4, FlexKit::SystemAllocator };
};

bool EditorTextureImporter::Import(const std::string fileDir)
{
	using namespace std::filesystem;

	path imagePath{ fileDir };

	if (!exists(imagePath))
		return false;

	int x;
	int y;
	int channelCount;
	uint8_t* imageBuffer	= stbi_load((const char*)fileDir.c_str(), &x, &y, &channelCount, 4);

	if (!imageBuffer)
		return false;

	auto& renderSystem	= renderer.GetRenderSystem();
	auto dialog = new TextureImporterDialog{ renderer, imageBuffer, { x, y }, (uint8_t)channelCount, renderSystem };

	return true;
}
