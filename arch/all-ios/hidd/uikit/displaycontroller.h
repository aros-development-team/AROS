#import <UIKit/UIKit.h>

@interface DisplayController : UIViewController
{
    unsigned char FixedOrientation;
}

@property(nonatomic) unsigned char FixedOrientation;

-(id)initWithSize:(CGRect)screenSize;

@end
