/*
 * Copyright 2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999 M.Kawamura
 */


#include "CannaCommon.h"
#include "PaletteWindow.h"
#if 0
#include "TangoWindow.h"
#endif
#include "canna/mfdef.h"
#include "PaletteIconImages.h"
#include <Bitmap.h>
#include <Box.h>
#include <Looper.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PictureButton.h>
#include <PopUpMenu.h>
#include <Screen.h>

#include <private/interface/WindowPrivate.h>

class BackBox:public BBox{
public:
	BackBox(BRect frame);
	virtual	void	MouseDown(BPoint point);
	virtual			~BackBox();
	virtual void	AttachedToWindow();
	BPopUpMenu*		fMenu;
};


BackBox::BackBox(BRect frame)
:BBox(frame, NULL,
	B_FOLLOW_LEFT | B_FOLLOW_TOP,
	B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
	B_FANCY_BORDER)
{
	fMenu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	fMenu->AddItem(new BMenuItem("About CannaIM" B_UTF8_ELLIPSIS,
		new BMessage( B_ABOUT_REQUESTED)));
	fMenu->AddSeparatorItem();
	fMenu->AddItem(new BMenuItem("Convert arrow keys",
		new BMessage(ARROW_KEYS_FLIPPED)));
	fMenu->AddItem(new BMenuItem("Reload Init file",
		new BMessage(RELOAD_INIT_FILE)));

	if (gSettings.convert_arrowkey) {
		BMenuItem* item = fMenu->FindItem(ARROW_KEYS_FLIPPED);
		item->SetMarked(true);
	}
}
BackBox::~BackBox()
{
	delete fMenu;
}


void
BackBox::AttachedToWindow()
{
	fMenu->SetTargetForItems(this->Window());
}

void
BackBox::MouseDown(BPoint point)
{
	BPoint where;
	where = ConvertToScreen(point);
	fMenu->Go(where,true);
}


PaletteWindow::PaletteWindow( BRect rect, BLooper *looper )
	:BWindow( rect, B_EMPTY_STRING, kLeftTitledWindowLook,
			B_FLOATING_ALL_WINDOW_FEEL,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE |
			B_AVOID_FOCUS | B_WILL_ACCEPT_FIRST_CLICK )
{
	cannaLooper = looper;

	ResizeTo( 122, 46 );
	BRect frame = Frame();
	frame.OffsetTo( -1, -1 );
	frame.bottom += 3;
	frame.right += 3;
	fBack = new BackBox( frame);
	AddChild( fBack );

	BRect largerect( 0, 0, HexOnwidth - 1, HexOnheight - 1 );
	BRect smallrect( 0, 0, HiraOnwidth - 1, HiraOnheight - 1);
	int32 largebytes = HexOnbytesperpixel * HexOnwidth * HexOnheight;
	int32 smallbytes = HiraOnbytesperpixel * HiraOnwidth * HiraOnheight;
	color_space cspace = HexOncspace;
	BPicture *onpict, *offpict;
	BBitmap *smallimage, *largeimage;
	BMessage *msg;
	smallimage = new BBitmap( smallrect, cspace );
	largeimage = new BBitmap( largerect, cspace );

	fBack->MovePenTo( 0, 0 );

	smallimage->SetBits( HiraOnbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	onpict = fBack->EndPicture();
	smallimage->SetBits( HiraOffbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_HenkanMode );
	HiraButton = new BPictureButton( BRect( 4, 4, 4 + HiraOnwidth - 1,
		4 + HiraOnheight - 1), "hira", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( HiraButton );

	smallimage->SetBits( KataOnbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	onpict = fBack->EndPicture();
	smallimage->SetBits( KataOffbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_ZenKataHenkanMode );
	KataButton = new BPictureButton( BRect( 26, 4, 26 + HiraOnwidth - 1,
		4 + HiraOnheight - 1 ), "kata", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( KataButton );

	smallimage->SetBits( ZenAlphaOnbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	onpict = fBack->EndPicture();
	smallimage->SetBits( ZenAlphaOffbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_ZenAlphaHenkanMode );
	ZenAlphaButton = new BPictureButton( BRect( 48, 4, 48 + HiraOnwidth - 1,
		4 + HiraOnheight - 1 ), "zenalpha", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( ZenAlphaButton );

	smallimage->SetBits( HanAlphaOnbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	onpict = fBack->EndPicture();
	smallimage->SetBits( HanAlphaOffbits, smallbytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( smallimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_HanAlphaHenkanMode );
	HanAlphaButton = new BPictureButton( BRect( 70, 4, 70 + HiraOnwidth - 1,
		4 + HiraOnheight - 1 ), "hanalpha", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( HanAlphaButton );

	largeimage->SetBits( ExtendOnbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	onpict = fBack->EndPicture();
	largeimage->SetBits( ExtendOffbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_TourokuMode );
	ExtendButton = new BPictureButton( BRect( 94, 4, 94 + HexOnwidth -1 ,
		4 + HexOnheight - 1 ), "extend", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( ExtendButton );

	largeimage->SetBits( KigoOnbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	onpict = fBack->EndPicture();
	largeimage->SetBits( KigoOffbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_KigoMode );
	KigoButton = new BPictureButton( BRect( 4, 26, 4 + HexOnwidth -1,
		26 + HexOnheight - 1 ), "kigo", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( KigoButton );

	largeimage->SetBits( HexOnbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	onpict = fBack->EndPicture();
	largeimage->SetBits( HexOffbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_HexMode );
	HexButton = new BPictureButton( BRect( 34, 26, 34 + HexOnwidth -1,
		26 + HexOnheight - 1 ), "hex", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( HexButton );

	largeimage->SetBits( BushuOnbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	onpict = fBack->EndPicture();
	largeimage->SetBits( BushuOffbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_BushuMode );
	BushuButton = new BPictureButton( BRect( 64, 26, 64 + HexOnwidth -1,
		26 + HexOnheight - 1 ), "bushu", offpict, onpict,
		msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( BushuButton );

#if 0
	largeimage->SetBits( TorokuOnbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	onpict = fBack->EndPicture();
	largeimage->SetBits( TorokuOffbits, largebytes, 0, cspace );
	fBack->BeginPicture( new BPicture );
	fBack->DrawBitmap( largeimage );
	offpict = fBack->EndPicture();
	msg = new BMessage( MODE_CHANGED_FROM_PALETTE );
	msg->AddInt32( "mode", CANNA_MODE_ExtendMode );
	TorokuButton = new BPictureButton( BRect( 87, 26, 110, 41 ), "toroku",
						offpict, onpict, msg, B_TWO_STATE_BUTTON );
	fBack->AddChild( TorokuButton );
#endif

	HiraButton->SetValue( B_CONTROL_ON );
	delete smallimage;
	delete largeimage;
	delete offpict;
	delete onpict;

}


void
PaletteWindow::MessageReceived( BMessage *msg )
{
	int32 mode;
	switch( msg->what )
	{
		case PALETTE_WINDOW_HIDE:
			if ( !IsHidden() )
				Hide();
				break;

		case PALETTE_WINDOW_SHOW:
			if ( IsHidden() )
			{
				BRect frame = Frame();
				BRect screenrect = BScreen().Frame();
				float x, y;

				x = screenrect.right - frame.right;
				y = screenrect.bottom - frame.bottom;

				if ( x < 0 )
					frame.OffsetBy( x, 0 );

				if ( y < 0 )
					frame.OffsetBy( 0, y );

				MoveTo( frame.left, frame.top );
				SetWorkspaces( B_CURRENT_WORKSPACE );
				Show();
			}
			break;

		case ARROW_KEYS_FLIPPED:
		{
			gSettings.convert_arrowkey = !gSettings.convert_arrowkey;
			BMenuItem* item = (fBack->fMenu)->FindItem(ARROW_KEYS_FLIPPED);
			item->SetMarked(gSettings.convert_arrowkey);
		}
		case B_ABOUT_REQUESTED:
		case RELOAD_INIT_FILE:
		case MODE_CHANGED_FROM_PALETTE:
			cannaLooper->PostMessage( msg );
			break;

		case PALETTE_WINDOW_BUTTON_UPDATE:
			msg->FindInt32( "mode", &mode );
			AllButtonOff();
			switch( mode )
			{
				case CANNA_MODE_ZenKataKakuteiMode:
				case CANNA_MODE_ZenKataHenkanMode:
					KataButton->SetValue( B_CONTROL_ON );
					break;
				case CANNA_MODE_ZenAlphaKakuteiMode:
				case CANNA_MODE_ZenAlphaHenkanMode:
					ZenAlphaButton->SetValue( B_CONTROL_ON );
					break;
				case CANNA_MODE_HanAlphaKakuteiMode:
				case CANNA_MODE_HanAlphaHenkanMode:
					HanAlphaButton->SetValue( B_CONTROL_ON );
					break;
				case CANNA_MODE_ExtendMode:
				case CANNA_MODE_TourokuMode:
				case CANNA_MODE_TourokuHinshiMode:
				case CANNA_MODE_TourokuDicMode:
				case CANNA_MODE_DeleteDicMode:
				case CANNA_MODE_MountDicMode:
					ExtendButton->SetValue( B_CONTROL_ON );
					break;
				case CANNA_MODE_KigoMode:
				case CANNA_MODE_RussianMode:
				case CANNA_MODE_GreekMode:
				case CANNA_MODE_LineMode:
					KigoButton->SetValue( B_CONTROL_ON );
					break;
				case CANNA_MODE_HexMode:
					HexButton->SetValue( B_CONTROL_ON );
					break;
				case CANNA_MODE_BushuMode:
					BushuButton->SetValue( B_CONTROL_ON );
					break;
/*
				case CANNA_MODE_TourokuMode:
				case CANNA_MODE_TourokuHinshiMode:
				case CANNA_MODE_TourokuDicMode:
				case CANNA_MODE_DeleteDicMode:
				case CANNA_MODE_MountDicMode:
					TorokuButton->SetValue( B_CONTROL_ON );
					break;
*/
				default:
					HiraButton->SetValue( B_CONTROL_ON );
					break;
			}
			break;


		default:
			BWindow::MessageReceived( msg );
	}
}


void
PaletteWindow::AllButtonOff()
{
	HiraButton->SetValue( B_CONTROL_OFF );
	KataButton->SetValue( B_CONTROL_OFF );
	ZenAlphaButton->SetValue( B_CONTROL_OFF );
	HanAlphaButton->SetValue( B_CONTROL_OFF );
	ExtendButton->SetValue( B_CONTROL_OFF );
	KigoButton->SetValue( B_CONTROL_OFF );
	HexButton->SetValue( B_CONTROL_OFF );
	BushuButton->SetValue( B_CONTROL_OFF );
#if 0
	TorokuButton->SetValue( B_CONTROL_OFF );
#endif
}


void
PaletteWindow::FrameMoved( BPoint screenPoint )
{
	gSettings.palette_loc = screenPoint;
}


