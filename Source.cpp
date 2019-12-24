#include <Windows.h>
#include <string>
#include <algorithm>
#include <chrono>
#include <vector>

using namespace std;

int screenWidth = 120;
int screenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;

int mapWidth = 16;
int mapHeight = 16;

float fFOV = 3.14159 / 4.0f;
float fDepth = 16.0f;

int main() {

	wchar_t *screen = new wchar_t[screenWidth * screenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;
	
	wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"#..............#";
	map += L"########.......#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.....##########";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	// Game Loop
	for (;;) {

		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Handle Movement
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
			fPlayerA -= (0.8f) * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
			fPlayerA += (0.8f) * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
			fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;

			// Collision detection
			if (map[(int)fPlayerY * mapWidth + (int)fPlayerX] == '#') {
				fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * mapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		for (int x = 0; x < screenWidth; x++) {

			// For each column, calculate the projected ray angle into world space
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)screenWidth) * fFOV;

			float fDistanceToWall = 0;
			bool hitWall = false;
			bool boundary = false;

			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			while (!hitWall && fDistanceToWall < fDepth) {
				fDistanceToWall += 0.1f;

				// Only need the integer part if we assume that walls will meet on integer boundaries
				int testX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int testY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (testX < 0 || testX >= mapWidth || testY < 0 || testY >= mapHeight) {
					hitWall = true;            // just set the distance to max depth
					fDistanceToWall = fDepth;
				}
				else {
					if (map[testY * mapWidth + testX] == '#') {
						hitWall = true;

						vector<pair<float, float>> p;  // distance, dot product
						for (int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {
								float vy = (float)testY + ty - fPlayerY;
								float vx = (float)testX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						}
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

						float fBound = 0.001;
						if (acos(p.at(0).second) < fBound) boundary = true;
						if (acos(p.at(1).second) < fBound) boundary = true;
						//if (acos(p.at(2).second) < fBound) boundary = true;
					}
				}
			}

			// calculate distance to ceiling and floor
			int ceiling = (float)(screenHeight / 2.0) - screenHeight / ((float)fDistanceToWall);
			int floor = screenHeight - ceiling;

			short shade = ' ';

			if (fDistanceToWall <= fDepth / 4.0f) {
				shade = 0x2588;
			}
			else if (fDistanceToWall < fDepth / 3.0f) {
				shade = 0x2593;
			}
			else if (fDistanceToWall < fDepth / 2.0f) {
				shade = 0x2592;
			}
			else if (fDistanceToWall < fDepth) {
				shade = 0x2591;
			}
			else {
				shade = ' ';
			}

			if (boundary) shade = ' ';

			for (int y = 0; y < screenHeight; y++) {
				if (y < ceiling) {
					screen[y * screenWidth + x] = ' ';
				}
				else if (y > ceiling && y <= floor) {
					screen[y * screenWidth + x] = shade;
				}
				else {
					short f = ' ';
					// shade floor based on distance
					float b = 1.0f - (((float)y - screenHeight / 2.0f) / ((float)screenHeight / 2.0f));

					if (b < 0.25) { f = '#'; }
					else if (b < 0.5) { f = 'X'; }
					else if (b < 0.75) { f = '.'; }
					else if (b < 0.9) { f = '-'; }
					else { f = ' '; }
					screen[y * screenWidth + x] = f;
				}
			}
		}
		swprintf(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

		// display map
		for (int nx = 0; nx < mapWidth; nx++) {
			for (int ny = 0; ny < mapWidth; ny++) {
				screen[(ny + 1) * screenWidth + nx] = map[ny * mapWidth + (mapWidth - nx - 1)];
			}
		}

		screen[((int)fPlayerY + 1) * screenWidth + (int)(mapWidth - fPlayerX)] = 'P';

		screen[screenWidth * screenHeight - 1] = '\0';
		WriteConsoleOutputCharacterW(hConsole, screen, screenWidth * screenHeight, { 0, 0 }, &dwBytesWritten);
	}

	
	return 0;
}