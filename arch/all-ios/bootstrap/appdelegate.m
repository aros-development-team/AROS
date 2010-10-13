#include <UIKit/UIAlertView.h>
#include <stdlib.h>

#include "appdelegate.h"
#include "bootstrap.h"

@implementation BootstrapDelegate

@synthesize alert;
@synthesize rc;

- (void)ShowAlert:(NSString *)text
{
    self.alert = [[UIAlertView alloc] initWithTitle:@"AROS bootstrap error" message:text delegate:self
    					   cancelButtonTitle:@"Abort" otherButtonTitles:nil];

    [self.alert show];
    [self.alert release];
}

+(void)DisplayAlert:(NSString *)text
{
    BootstrapDelegate *ad = (BootstrapDelegate *)[[UIApplication sharedApplication] delegate];

    [ad ShowAlert:text];
}

- (void)RunBootstrap
{
    self.rc = bootstrap(_argc, _argv);

    /* Fall down into event loop. We have alert view here. */
}

- (void)alertView:(UIAlertView *)av didDismissWithButtonIndex:(NSInteger)choice
{
    /* I know it's considered bad. But what else to do here? Restart? */
    exit(self.rc);
}

- (void)applicationDidFinishLaunching:(UIApplication *)app
{
    /*
     * This causes RunBootstrap method to be called after all internal UIKit setup is complete,
     * when event handling loop is entered
     */
    [self performSelector:@selector(RunBootstrap) withObject:nil afterDelay:0.0];
}

/* The following are placeholders for now. Should likely be forwarded to AROS via HostInterface. */

- (void)applicationWillTerminate:(UIApplication *)application
{

}

- (void) applicationWillResignActive:(UIApplication*)application
{

}

- (void) applicationDidBecomeActive:(UIApplication*)application
{

}

@end
