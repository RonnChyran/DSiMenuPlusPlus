#ifndef ICONTITLE_H
#define ICONTITLE_H

#define BOX_PX				22
#define BOX_PY				4
#define BOX_PY_spacing1		13
#define BOX_PY_spacing2		6
#define BOX_PY_spacing3		19

void iconTitleInit();
void loadConsoleIcons();
void getGameInfo(int num, bool isDir, const char* name);
void iconUpdate(int num, bool isDir, const char* name);
void titleUpdate(int num, bool isDir, const char* name);
void drawIcon(int num, int Xpos, int Ypos);
void drawIconPCE(int Xpos, int Ypos);
void drawIconA26(int Xpos, int Ypos);
void drawIconPlg(int Xpos, int Ypos);
void drawIconGBA(int Xpos, int Ypos);
void drawIconGB(int Xpos, int Ypos);
void drawIconGBC(int Xpos, int Ypos);
void drawIconNES(int Xpos, int Ypos);
void drawIconSMS(int Xpos, int Ypos);
void drawIconGG(int Xpos, int Ypos);
void drawIconMD(int Xpos, int Ypos);
void drawIconSNES(int Xpos, int Ypos);

#endif // ICONTITLE_H