#import "alertdelegate.h"

@implementation AlertDelegate

- (void)DisplayAlert:(NSString *)text
{
    /* Open alert view first */
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"AROS GURU meditation" message:text delegate:self
    					      cancelButtonTitle:@"Abort" otherButtonTitles:nil];
    [alert show];

    /* 
     * Enter run loop in order to process events from the alert view
     * TODO: we want to be able to return when the alert is dismissed
     */
    NSRunLoop *rl = [NSRunLoop mainRunLoop];
    [rl run];

    [alert release];
}

- (void)alertView:(UIAlertView *)av didDismissWithButtonIndex:(NSInteger)choice
{
    /* TODO: Break run loop here */
    exit(0);
}

@end
