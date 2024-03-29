#include "gui.h"


class cSearchButton;


class GuiSearchBar : public GuiWindow
{
public:
	GuiSearchBar(const wchar_t *SearchChars);
	~GuiSearchBar();
	void Draw();
	void Update(GuiTrigger * t);
	wchar_t GetClicked();
private:
	u16				inSide;
	
	GuiText			text;
	
	GuiImageData*	imgBacspaceBtn;
	GuiImage*		BacspaceBtnImg;
	GuiImage*		BacspaceBtnImg_Over;
	GuiButton*		BacspaceBtn;
	
	GuiImageData*	imgClearBtn;
	GuiImage*		ClearBtnImg;
	GuiImage*		ClearBtnImg_Over;
	GuiButton*		ClearBtn;
	
	cSearchButton	**buttons;
	int				cnt;
	GuiImageData	keyImageData;
	GuiImageData	keyOverImageData;
	GuiTrigger		trig;
	GuiSound		sndOver;
	GuiSound		sndClick;

};
