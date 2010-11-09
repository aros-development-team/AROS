#import <UIKit/UIKit.h>

extern int    _argc;
extern char **_argv;

@interface BootstrapDelegate:NSObject<UIApplicationDelegate, UIAlertViewDelegate>
{
@private
    UIAlertView *alert;
    int rc;
}

@property(nonatomic, retain) UIAlertView *alert;
@property int rc;

- (void)ShowAlert:(NSString *)text withTitle:(NSString *)title;

@end
