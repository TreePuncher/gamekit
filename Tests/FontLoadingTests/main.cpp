#include <print>
#include <ft2build.h>
#include <vector>
#include <SFML/Graphics.hpp>
#include <ranges>
#include <MathUtilities.hpp>

#include FT_FREETYPE_H
#include FT_IMAGE_H


using namespace FlexKit;
using namespace std;

struct glyphWindow
{
	uint32_t w;
	uint32_t h;
};

struct BezierCurve
{
	uint2 p0;
	uint2 p1;
	uint2 p2;
};

BezierCurve LineToBezierCurve(const uint2& a, const uint2& b) noexcept
{
	return BezierCurve{
		.p0 = a,
		.p1{
			a[0] / 2 + b[0] / 2,
			a[1] / 2 + b[1] / 2,
		},
		.p2 = b,
	};
}

struct Glyph
{
	glyphWindow					window;
	std::vector<uint2>			points;
	std::vector<BezierCurve>	curves;
};

int main()
{
	FT_Library library;
	if (auto rc = FT_Init_FreeType(&library); rc)
	{
		return -1;
	}

	FT_Face face;
	if (auto rc = FT_New_Face(library, "test.otf", -1, &face); rc == FT_Err_Unknown_File_Format)
	{
		return -2;
	}
	else if (rc)
	{
		return -3;
	}

	uint32_t faceCount = face->num_faces;
	print("face count : {}", faceCount);

	FT_Done_Face(face);

	std::vector<Glyph> glyphs;

	for (uint32_t i = 0; i < faceCount; i++)
	{
		auto rc = FT_New_Face(library, "test.otf", i, &face);
		if (rc == 0)
		{
			const uint32_t glyphCount = face->num_glyphs;
			print("glpyh count : {}", glyphCount);


			for (uint32_t ii = 0; ii < glyphCount; ii++)
			{
				if (auto res = FT_Load_Glyph(face, ii, FT_LOAD_NO_SCALE); res != 0)
				{
					return -2;
				}

				Glyph g;
				if (face->glyph->outline.points)
				{
					const uint32_t pointCount = face->glyph->outline.n_points;
					for (uint32_t j = 0; j < pointCount; j++)
					{
						auto point = face->glyph->outline.points[j];
						const int32_t x = point.x;
						const int32_t y = point.y;

						g.points.emplace_back(x, y);
					}

					glyphs.push_back(std::move(g));
				}
			}

			FT_Done_Face(face);
		}
	}

	FT_Done_FreeType(library);

	if (glyphs.size())
	{
		uint32_t glyphidx = 2;

		sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "My window");
		window.create(sf::VideoMode({ 800, 600 }), "My window");


		std::vector<sf::Vertex> outline;

		auto loadOutline = [&]
			{
				outline.clear();

				auto& g = glyphs[glyphidx];

				if (g.points.size() == 0)
					return;

				for (uint32_t i = 0; i < g.points.size() - 1; i++)
				{
					auto& p0 = g.points[i];
					auto& p1 = g.points[i + 1];
					sf::Vertex v;
					v.color = sf::Color::White;
					v.position.x = 10.0f * float(p0[0] * (1.0f / 64.0f)) + 400.0f;
					v.position.y = 300.0f - 10.0f * float(p0[1] * (1.0f / 64.0f));

					outline.push_back(v);

					v.position.x = 10.0f * float(p1[0] * (1.0f / 64.0f)) + 400.0f;
					v.position.y = 300.0f - 10.0f * float(p1[1] * (1.0f / 64.0f));
					outline.push_back(v);
				}

				{
					auto& p0 = g.points[g.points.size() - 1];
					auto& p1 = g.points[0];

					sf::Vertex v;
					v.color = sf::Color::White;
					v.position.x = 10.0f * float(p0[0] * (1.0f / 64.0f)) + 400.0f;
					v.position.y = 300.0f - 10.0f * float(p0[1] * (1.0f / 64.0f));

					outline.push_back(v);
					v.position.x = 10.0f * float(p1[0] * (1.0f / 64.0f)) + 400.0f;
					v.position.y = 300.0f - 10.0f * float(p1[1] * (1.0f / 64.0f));
					outline.push_back(v);
				}

			};

		const float scale = 2.0f;

		auto loadOutlines = [&]
			{
				outline.clear();

				uint32_t x = 0;
				uint32_t y = 0;

				for (auto&& [idx, g] : std::views::zip(std::views::iota(0), glyphs))
				{
					if (g.points.size() == 0)
						continue;

					float x_start = 0 + 20.0f * (idx % 20) * scale;
					float y_start = 25 + 20.0f * (idx / 20) * scale;

					for (uint32_t i = 0; i < g.points.size() - 1; i++)
					{
						auto& p0 = g.points[i];
						auto& p1 = g.points[i + 1];
						sf::Vertex v;
						v.color = sf::Color::Red;
						v.position.x = x_start + float(p0[0] * (1.0f / 64.0f)) * scale;
						v.position.y = y_start - float(p0[1] * (1.0f / 64.0f)) * scale;
						outline.push_back(v);

						v.color = sf::Color::Green;
						v.position.x = x_start + float(p1[0] * (1.0f / 64.0f)) * scale;
						v.position.y = y_start - float(p1[1] * (1.0f / 64.0f)) * scale;
						outline.push_back(v);
					}

					{
						auto& p0 = g.points[g.points.size() - 1];
						auto& p1 = g.points[0];

						sf::Vertex v;
						v.color = sf::Color::Red;
						v.position.x = x_start + float(p0[0] * (1.0f / 64.0f)) * scale;
						v.position.y = y_start - float(p0[1] * (1.0f / 64.0f)) * scale;
						outline.push_back(v);

						v.color = sf::Color::Green;
						v.position.x = x_start + float(p1[0] * (1.0f / 64.0f)) * scale;
						v.position.y = y_start - float(p1[1] * (1.0f / 64.0f)) * scale;
						outline.push_back(v);
					}
				}

			};

		loadOutlines();

		while (window.isOpen())
		{
			while (const std::optional event = window.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
					window.close();

				if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
				{
					if (keyPressed->scancode == sf::Keyboard::Scan::Space)
					{
						glyphidx = (++glyphidx) % (glyphs.size());
						loadOutline();
					}
				}
			}

			window.clear();
			window.draw(outline.data(), outline.size(), sf::PrimitiveType::Lines);

			window.display();
		}

	}
}
