#ifndef _ABOUTWINDOW_PRIVATE_H_
#define _ABOUTWINDOW_PRIVATE_H_

/*** Instance data **********************************************************/
struct AboutWindow_DATA
{
    /*- Private ------------------------------------------------------------*/
    struct Catalog *awd_Catalog;
    
    /*- Protected ----------------------------------------------------------*/
    Object         *awd_RootGroup,
                   *awd_ImageGroup,
                   *awd_ImageObject,
                   *awd_VersionObject,
                   *awd_CopyrightObject,
                   *awd_DescriptionGroup,
                   *awd_DescriptionObject;
                   
    /*- Public -------------------------------------------------------------*/
    STRPTR          awd_Title,
                    awd_VersionNumber,
                    awd_VersionDate,
                    awd_VersionExtra,
                    awd_Copyright,
                    awd_Description;
};

#endif /* _ABOUTWINDOW_PRIVATE_H_ */
