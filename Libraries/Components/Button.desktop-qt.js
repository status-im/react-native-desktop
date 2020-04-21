/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @format
 * @flow
 */

'use strict';

const Platform = require('Platform');
const React = require('React');
const StyleSheet = require('StyleSheet');
const Text = require('Text');
const TouchableNativeFeedback = require('TouchableNativeFeedback');
const TouchableOpacity = require('TouchableOpacity');
const View = require('View');

const invariant = require('fbjs/lib/invariant');

import type {PressEvent} from 'CoreEventTypes';

type ButtonProps = $ReadOnly<{|
  /**
   * Text to display inside the button
   */
  title: string,

  /**
   * Handler to be called when the user taps the button
   */
  onPress: (event?: PressEvent) => mixed,

  /**
   * Color of the text (iOS), or background color of the button (Android)
   */
  color?: ?string,

  /**
   * TV preferred focus (see documentation for the View component).
   */
  hasTVPreferredFocus?: ?boolean,

  /**
   * Text to display for blindness accessibility features
   */
  accessibilityLabel?: ?string,

  /**
   * If true, disable all interactions for this component.
   */
  disabled?: ?boolean,

  /**
   * Used to locate this view in end-to-end tests.
   */
  testID?: ?string,
|}>;

/**
 * A basic button component that should render nicely on any platform. Supports
 * a minimal level of customization.
 *
 * <center><img src="img/buttonExample.png"></img></center>
 *
 * If this button doesn't look right for your app, you can build your own
 * button using [TouchableOpacity](docs/touchableopacity.html)
 * or [TouchableNativeFeedback](docs/touchablenativefeedback.html).
 * For inspiration, look at the [source code for this button component](https://github.com/facebook/react-native/blob/master/Libraries/Components/Button.js).
 * Or, take a look at the [wide variety of button components built by the community](https://js.coach/react-native?search=button).
 *
 * Example usage:
 *
 * ```
 * import { Button } from 'react-native';
 * ...
 *
 * <Button
 *   onPress={onPressLearnMore}
 *   title="Learn More"
 *   color="#841584"
 *   accessibilityLabel="Learn more about this purple button"
 * />
 * ```
 *
 */

class Button extends React.Component<ButtonProps> {
  render() {
    const {
      accessibilityLabel,
      color,
      onPress,
      title,
      hasTVPreferredFocus,
      disabled,
      testID,
    } = this.props;
    const buttonStyles = [styles.button];
    const textStyles = [styles.text];
    if (color) {
      if (Platform.OS === 'ios' || Platform.OS === 'desktop') {
        textStyles.push({color: color});
      } else {
        buttonStyles.push({backgroundColor: color});
      }
    }
    const accessibilityStates = [];
    if (disabled) {
      buttonStyles.push(styles.buttonDisabled);
      textStyles.push(styles.textDisabled);
      accessibilityStates.push('disabled');
    }
    invariant(
      typeof title === 'string',
      'The title prop of a Button must be a string',
    );
    const formattedTitle =
      Platform.OS === 'android' ? title.toUpperCase() : title;
    const Touchable =
      Platform.OS === 'android' ? TouchableNativeFeedback : TouchableOpacity;
    return (
      <Touchable
        accessibilityLabel={accessibilityLabel}
        accessibilityRole="button"
        accessibilityStates={accessibilityStates}
        hasTVPreferredFocus={hasTVPreferredFocus}
        testID={testID}
        disabled={disabled}
        onPress={onPress}>
        <View style={buttonStyles}>
          <Text style={textStyles} disabled={disabled}>
            {formattedTitle}
          </Text>
        </View>
      </Touchable>
    );
  }
}

const styles = StyleSheet.create({
  button: Platform.select({
    ios: {},
    android: {
      elevation: 4,
      // Material design blue from https://material.google.com/style/color.html#color-color-palette
      backgroundColor: '#2196F3',
      borderRadius: 2,
    },
  }),
  text: {
    ...Platform.select({
      ios: {
        // iOS blue from https://developer.apple.com/ios/human-interface-guidelines/visual-design/color/
        color: '#007AFF',
        textAlign: 'center',
        padding: 8,
        fontSize: 18,
      },
      android: {
        color: 'white',
        textAlign: 'center',
        padding: 8,
        fontWeight: '500',
      },
      desktop: {
        color: '#007AFF',
        textAlign: 'center',
        fontSize: 14,
    },
    }),
  },
  buttonDisabled: Platform.select({
    ios: {},
    android: {
      elevation: 0,
      backgroundColor: '#dfdfdf',
    },
  }),
  textDisabled: Platform.select({
    ios: {
      color: '#cdcdcd',
    },
    android: {
      color: '#a1a1a1',
    },
    desktop: {
      color: '#cdcdcd',
    },
  }),
});

module.exports = Button;
