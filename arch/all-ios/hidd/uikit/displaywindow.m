#import "displaywindow.h"

@implementation DisplayWindow

-(id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        controller = [[DisplayController alloc] initWithSize:frame];
        
        [self addSubview:controller.view];
    }
    return self;
}

-(void)dealloc
{
    [controller release];
    
    [super dealloc];
}

@end
