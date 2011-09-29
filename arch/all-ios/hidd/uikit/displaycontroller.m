#import "displaycontroller.h"

@implementation DisplayController

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
    return Portrait ? UIInterfaceOrientationIsPortrait(o) : UIInterfaceOrientationIsLandscape(o);
}

@end
