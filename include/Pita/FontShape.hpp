#pragma once
#pragma warning(disable:4996)
#include "Vectorizer.hpp"

struct stbtt_fontinfo;

namespace cgl
{
	static unsigned char current_buffer[1 << 25];

	bool IsClockWise(const Vector<Eigen::Vector2d>& closedPath);

	//最後の点は含めない
	void GetQuadraticBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, int n);

	class FontBuilder
	{
	public:
		FontBuilder();
		FontBuilder(const std::string& fontPath);
		~FontBuilder();

		std::vector<gg::Geometry*> makePolygon(int codePoint, int quality = 1, double offsetX = 0, double offsetY = 0);

		std::vector<gg::Geometry*> textToPolygon(const std::string& str, int quality = 1);

		double glyphWidth(int codePoint);

	private:
		std::string fontDataRawEN, fontDataRawJP;
		stbtt_fontinfo *fontInfo1, *fontInfo2;
		int ascent1, descent1, lineGap1;
		int ascent2, descent2, lineGap2;
	};
}

