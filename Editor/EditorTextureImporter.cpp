#include "PCH.h"
#include "DXRenderWindow.h"
#include "EditorProject.h"
#include "EditorRenderer.h"
#include "EditorTextureImporter.h"
#include "EditorTextureResources.h"
#include "EditorTextureViewer.h"
#include "qlayout.h"
#include <stb_image.h>


using namespace FlexKit;

EditorTextureImporter::EditorTextureImporter(EditorProject& IN_proj, EditorRenderer& IN_renderer) :
	project		{ IN_proj		},
	renderer	{ IN_renderer	}
{
}

struct TextureImporterDialog
{
	TextureImporterDialog(EditorRenderer& renderer, void* _ptr, uint2 wh, uint8_t channelCount, IRenderSystem& renderSystem)
	{
		size_t rowPitch		= FlexKit::AlignedSize(wh[0] * 4);
		size_t bufferSize	= rowPitch * wh[1];

		converted = TextureBuffer{ wh, 4, bufferSize , FlexKit::SystemAllocator };

		TextureBuffer buffer					{ wh, (std::byte*)_ptr, 3 };
		TextureBufferView<RGB>		inputView	{ buffer };
		TextureBufferView<RGBA>		outputView	{ converted, rowPitch };

		memset(converted.Buffer, 0, converted.BufferSize());

		for (size_t y_itr = 0; y_itr < wh[1]; y_itr++)
		{
			for (size_t x_itr = 0; x_itr < wh[0]; x_itr++)
			{
				FlexKit::uint2 px = { x_itr, y_itr };
				outputView[px] = inputView[px];
			}
		}

		
		auto textureHandle = renderSystem.LoadTexture(&converted, renderSystem.GetImmediateCopyQueue(), DeviceFormat::R8G8B8A8_UNORM, SystemAllocator);

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
	FlexKit::ResourceHandle	texture			= InvalidHandle;
	Ui_TextureImportDialog	UI;
	TextureBuffer			converted;
};

bool EditorTextureImporter::Import(const std::string& fileDir)
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
