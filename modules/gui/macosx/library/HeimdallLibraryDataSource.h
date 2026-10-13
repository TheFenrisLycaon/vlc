//
//  HeimdallLibraryDataSource.h
//  Heimdall
//
//  Created by Claudio Cambra on 9/9/24.
//

#ifndef HeimdallLibraryDataSource_h
#define HeimdallLibraryDataSource_h

#import <Foundation/Foundation.h>

@protocol HeimdallLibraryDataSource <NSObject>

@optional
- (void)connect;
- (void)disconnect;

@end

#endif /* HeimdallLibraryDataSource_h */
