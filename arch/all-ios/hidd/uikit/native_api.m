#import <objc/runtime.h>
#import <UIKit/UIKit.h>

#import "alertdelegate.h"
#import "displaywindow.h"
#import "native_api.h"
#import "bitmapclass.h"

static UIScreen *getScreen(unsigned int scrNo)
{
    void *ios32 = class_getClassMethod([UIScreen class], @selector(screens));

    if (ios32)
    {
    	NSArray *screens = [UIScreen screens];
    	
    	if (scrNo < [screens count])
    	    return [screens objectAtIndex:scrNo];
    	else
    	    return nil;
    }
    else
    {
    	if (scrNo == 0)
    	    return [UIScreen mainScreen];
    	else
    	    return nil;
    }
}

/*
 * This function is used to get logical metrics of the screen. It returns total screen
 * size and status bar size in *POINTS*.
 * The mechanics behind: physical iOS screen always has (0, 0) in the same place. It's
 * not changed by rotation. Rotation is handled by UIViewController on view level by
 * applying rotation transformation to the underlying view.
 * Now, bu default, when the application has just started, we have some screen size
 * and we want to know what space will be occupied by screen bar in both variants
 * (portrait and landscape). The OS provides us with statusBarFrame for this
 * purpose. Actually, status bar also doesn't rotate. It's just drawn rotated by 90
 * degrees when needed. Its frame is always represented in screen's coordinates.
 * By default status bar is placed on the top, occupying the whole screen width, and
 * having some height. In rotated position, it occupies the *whole height*, but only
 * *some* width.
 * In this code we detect screenbar position, just in case, to make sure we provide
 * the correct coordinate. The only assumption here is that other size member will
 * have full-screen size. This seems to be always true.
 *
 * This function returns sizes in points, not in pixels. In order to move from points
 * to pixels, we'll have to scale screenbar size according to pixels:points relation
 * for every mode.
 */
void GetMetrics(struct DisplayMetrics *data)
{
    UIScreen *screen = [UIScreen mainScreen];
    CGRect screenbar = [UIApplication sharedApplication].statusBarFrame;
    UIInterfaceOrientation o = [UIApplication sharedApplication].statusBarOrientation;

    data->width       = screen.bounds.size.width;
    data->height      = screen.bounds.size.height;
    
    if (UIInterfaceOrientationIsLandscape(o))
    {
    	data->orientation = O_LANDSCAPE;
    	data->screenbar   = screenbar.size.width;
    }
    else
    {
    	data->orientation = O_PORTRAIT;
    	data->screenbar   = screenbar.size.height;
    }
}

DisplayWindow *OpenDisplay(unsigned int scrNo)
{
    UIScreen *screen = getScreen(scrNo);
    DisplayWindow *win;

    if (!screen)
    	return nil;

    win = [[DisplayWindow alloc] initWithFrame:screen.bounds];
    [win makeKeyAndVisible];

    return win;
}

void CloseDisplay(DisplayWindow *win)
{
    [win release];
}

UIView *NewBitMap(DisplayWindow *win, unsigned int w, unsigned int h)
{
    UIView *bmView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, w, h)];

    [win addSubview:bmView];
    return bmView;
}

void DisposeBitMap(UIView *bitmap)
{
    [bitmap removeFromSuperview];
    [bitmap release];
}

void DisplayAlert(const char *text)
{
    AlertDelegate *ad = [[AlertDelegate alloc] init];
    NSString *alert = [NSString stringWithCString:text encoding:NSISOLatin1StringEncoding];

    [ad DisplayAlert:alert];
    [ad release];
}

void NewContext(struct bitmap_data *bitmap)
{
    CGColorSpaceRef colspace = CGColorSpaceCreateDeviceRGB();

    bitmap->context = CGBitmapContextCreate(bitmap->pixels, bitmap->width, bitmap->height, 8, bitmap->mod, colspace, kCGImageAlphaNoneSkipFirst);
    CGColorSpaceRelease(colspace);
}

void DisposeContext(CGContextRef ctx)
{
    CGContextRelease(ctx);
}
