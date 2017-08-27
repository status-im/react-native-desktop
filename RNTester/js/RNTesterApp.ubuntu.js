/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * @providesModule RNTesterApp
 * @flow
 */

'use strict';

const AsyncStorage = require('AsyncStorage');
const BackHandler = require('BackHandler');
const Linking = require('Linking');
const React = require('react');
const ReactNative = require('react-native');
const RNTesterActions = require('./RNTesterActions');
const RNTesterExampleContainer = require('./RNTesterExampleContainer');
const RNTesterExampleList = require('./RNTesterExampleList');
const RNTesterList = require('./RNTesterList.ubuntu');
const RNTesterNavigationReducer = require('./RNTesterNavigationReducer');
const URIActionMap = require('./URIActionMap');


const {
  AppRegistry,
  StyleSheet,
  Text,
  View,
  Image
} = ReactNative;

import type { RNTesterExample } from './RNTesterList.ubuntu';
import type { RNTesterAction } from './RNTesterActions';
import type { RNTesterNavigationState } from './RNTesterNavigationReducer';

type Props = {
  exampleFromAppetizeParams: string,
};

const APP_STATE_KEY = 'RNTesterAppState.v2';

const Header = ({ onBack, title }: { onBack?: () => mixed, title: string }) => (
  <View style={styles.header}>
    <View style={styles.headerCenter}>
      <Text style={styles.title}>{title}</Text>
    </View>
    {onBack && <View style={styles.headerLeft}>
      <Button title="Back" onPress={onBack} />
    </View>}
  </View>
);

// class RNTesterApp extends React.Component {
//   render() {
//     return (
//       <Image
//         borderTopLeftRadius={20}
//         source={{uri: 'https://facebook.github.io/react-native/img/favicon.png', scale: 2}}
//         style={{width: 100, height: 100}}
//         testID={'testImage'}
//         onLoadStart={() => console.log("Image.onLoadStart()")}
//         onLoadEnd={() => console.log("Image.onLoadEnd()")}
//         onLoad={() => console.log("Image.onLoad()")}
//         onError={() => console.log("Image.onError()")}
//         onProgress={() => console.log("Image.onProgress()")}
//         />
//     );
//   }
// }

class RNTesterApp extends React.Component {
  props: Props;
  state: RNTesterNavigationState;
  //state: RNTesterNavigationState{ openExample: 'ImageExample'}

  componentWillMount() {
    //BackHandler.addEventListener('hardwareBackPress', this._handleBack);
  }

  componentDidMount() {
    console.log('!!! set new state')
    let action = RNTesterActions.ExampleAction('ImageExample');
    const newState = RNTesterNavigationReducer({
      openExample: 'ImageExample',
    }, action);
    this.setState(
      newState,
      () => {}
    );

    // if (this.state !== newState) {
    //   this.setState(
    //     newState,
    //     () => AsyncStorage.setItem(APP_STATE_KEY, JSON.stringify(this.state))
    //   );
    // }

    // this.setState(RNTesterNavigationReducer(this.state, 'ImageExample'));
    // Linking.getInitialURL().then((url) => {
    //   AsyncStorage.getItem(APP_STATE_KEY, (err, storedString) => {
    //     const exampleAction = URIActionMap(this.props.exampleFromAppetizeParams);
    //     const urlAction = URIActionMap(url);
    //     const launchAction = exampleAction || urlAction;
    //     if (err || !storedString) {
    //       const initialAction = launchAction || {type: 'InitialAction'};
    //       this.setState(RNTesterNavigationReducer(undefined, initialAction));
    //       return;
    //     }
    //     const storedState = JSON.parse(storedString);
    //     if (launchAction) {
    //       this.setState(RNTesterNavigationReducer(storedState, launchAction));
    //       return;
    //     }
    //     this.setState(storedState);
    //   });
    // });
    //
    // Linking.addEventListener('url', (url) => {
    //   this._handleAction(URIActionMap(url));
    // });
  }

  _handleBack = () => {
    this._handleAction(RNTesterActions.Back());
  }

  _handleAction = (action: ?RNTesterAction) => {
    if (!action) {
      return;
    }
    const newState = RNTesterNavigationReducer(this.state, action);
    if (this.state !== newState) {
      this.setState(
        newState,
        () => AsyncStorage.setItem(APP_STATE_KEY, JSON.stringify(this.state))
      );
    }
  }
  // render() {
  //   return (
  //     const Component = RNTesterList.Modules['./ImageExample'];
  //     <View style={styles.exampleContainer}>
  //         <Header onBack={this._handleBack} title={Component.title} />
  //         <RNTesterExampleContainer module={Component} />
  //     </View>
  //   );
  // }

  render() {
    console.log('!!!! render state:', this.state)
    if (!this.state) {
      return null;
    }
    if (this.state.openExample) {
      const Component = RNTesterList.Modules[this.state.openExample];
      if (Component.external) {
        return (
          <Component
            onExampleExit={this._handleBack}
          />
        );
      } else {
        return (
          <View style={styles.exampleContainer}>
            <Header onBack={this._handleBack} title={Component.title} />
            <RNTesterExampleContainer module={Component} />
          </View>
        );
      }

    }
    return (
      <View style={styles.exampleContainer}>
        <Header title="RNTester" />
        <RNTesterExampleList
          onNavigate={this._handleAction}
          list={RNTesterList}
        />
      </View>
    );
  }
}

const styles = StyleSheet.create({
  header: {
    height: 60,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#96969A',
    backgroundColor: '#F5F5F6',
    flexDirection: 'row',
    paddingTop: 20,
  },
  headerLeft: {
  },
  headerCenter: {
    flex: 1,
    position: 'absolute',
    top: 27,
    left: 0,
    right: 0,
  },
  title: {
    fontSize: 19,
    fontWeight: '600',
    textAlign: 'center',
  },
  exampleContainer: {
    flex: 1,
  },
});


AppRegistry.registerComponent('RNTesterApp', () => RNTesterApp)
