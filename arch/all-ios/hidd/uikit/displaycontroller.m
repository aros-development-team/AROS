#import "displaycontroller.h"
#import "native_api.h"

@implementation DisplayController

@synthesize FixedOrientation;

-(id)initWithSize:(CGRect)screenSize
{
    self = [super init];
    if (self)
    {
    	UIView *rootView =  [[UIView alloc] initWithFrame:screenSize];

    	self.view = rootView;
    	[rootView release];
    }
    return self;
}

-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)o
{
    switch (FixedOrientation)
    {
    case O_PORTRAIT:
    	return UIInterfaceOrientationIsPortrait(o);

    case O_LANDSCAPE:
    	return UIInterfaceOrientationIsLandscape(o);

    default:	/* Unspecified */
    	return YES;
    }
}

@end
