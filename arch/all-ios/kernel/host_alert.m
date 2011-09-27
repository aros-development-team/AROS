#import <UIKit/UIKit.h>

#include "host_alert.h"

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

/* This is our interface function */
void DisplayAlert(char *text)
{
    AlertDelegate *ad = [[AlertDelegate alloc] init];
    NSString *alert = [NSString stringWithCString:text encoding:NSISOLatin1StringEncoding];

    [ad DisplayAlert:alert];
    [ad release];
}
