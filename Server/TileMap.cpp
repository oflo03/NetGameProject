#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "TileMap.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
	std::filesystem::path FindMapPath(int stage)
	{
		const auto name = "map" + std::to_string(stage) + ".txt";
		wchar_t modulePath[32768]{};
		const auto length = GetModuleFileNameW(nullptr, modulePath, 32768);
		std::error_code error;
		if (length > 0 && length < 32768) {
			const auto directory = std::filesystem::path(modulePath).parent_path();
			// 배포 폴더와 솔루션의 x64/{Debug,Release} 실행을 지원한다.
			for (const auto& folder : { directory / "resources", directory / "../../Client/resources" }) {
				const auto path = folder / name;
				if (std::filesystem::is_regular_file(path, error))
					return path.lexically_normal();
			}
		}
		return std::filesystem::path("../Client/resources") / name;
	}
}

bool TileMap::Load(int stage)
{
	// [S9] TileMap::Load: 벽·물 충돌 정보를 모두 읽은 뒤 교체한다.
	if (stage < 1 || stage > 4)
		return false;
	const auto path = FindMapPath(stage);
	std::ifstream file(path);
	if (!file) {
		std::cerr << "TileMap: cannot open " << path << '\n';
		return false;
	}
	std::vector<BlockedTile> loaded;
	for (int row = 0; row < Rows; ++row) {
		for (int column = 0; column < Columns; ++column) {
			int type = 0, tilePos = 0;
			if (!(file >> type >> tilePos) || type < 0 || type > WaterType || tilePos < 0) {
				std::cerr << "TileMap: invalid tile at " << row << ',' << column << " in " << path << '\n';
				return false;
			}
			if (type == WallType || type == WaterType) {
				BlockedTile tile;
				tile.type = type;
				tile.pos = Vector2D<float>(static_cast<float>(column * TileExtent + HalfExtent),
					static_cast<float>(row * TileExtent + HalfExtent));
				loaded.push_back(tile);
			}
		}
	}
	file >> std::ws;
	if (!file.eof()) {
		std::cerr << "TileMap: extra data in " << path << '\n';
		return false;
	}
	blockedTiles.swap(loaded);
	return true;
}
