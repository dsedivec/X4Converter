#pragma once

#include <assimp/StreamReader.h>
#include <boost/format.hpp>
#include <iostream>
#include <exception>
class AniKeyframe {
public:
    AniKeyframe() = default;

    explicit AniKeyframe(Assimp::StreamReader<> &reader);


    std::string validate();// Debug method - throws exception if invalid, else returns human readable string
protected:
    // Note that these add up to exactly 128 bytes
    float ValueX, ValueY, ValueZ;                        /**< The key's actual value (position, rotation, etc.). 12*/
    int InterpolationX;                                    /**< The type of interpolation for the x part of the key. 20*/
    int InterpolationY;                                    /**< The type of interpolation for the y part of the key. 24*/
    int InterpolationZ;                                    /**< The type of interpolation for the z part of the key. 28*/
    float Time;                                            /**< 32 Time in s of the key frame - based on the start of the complete animation. - This value is also used as a unique identifier for the key meaning that two keys with the same time are considered the same! - We use a float rather than an XTIME to safe memory, because floating point precision is good enough for key times. */

    float CPX1x, CPX1y;                                    /**< First control point for the x value. 8*/
    float CPX2x, CPX2y;                                    /**< Second control point for the x value. 16*/
    float CPY1x, CPY1y;                                /**< First control point for the y value. 24*/
    float CPY2x, CPY2y;                                /**< Second control point for the y value. 32*/
    float CPZ1x, CPZ1y;                                /**< First control point for the z value. 40*/
    float CPZ2x, CPZ2y;                                /**< Second control point for the z value. 48*/

    float Tens;                                            /**< Tension. 4*/
    float Cont;                                            /**< Continuity. 8*/
    float Bias;                                            /**< Bias. 12*/
    float EaseIn;                                        /**< Ease In. 16*/
    float EaseOut;                                        /**< Ease Out. 20*/
    int Deriv;                                            /**< 24 Indicates whether derivatives have been calculated already. Is mutable to allow it being altered in the CalculateDerivatives() method. */
    float DerivInX, DerivInY, DerivInZ;                    /**< 12 Derivative In value. Is mutable to allow it being altered in the CalculateDerivatives() method. */
    float DerivOutX, DerivOutY, DerivOutZ;                /**< 24 Derivative Out value.  Is mutable to allow it being altered in the CalculateDerivatives() method.*/
    unsigned int AngleKey;                                        /** 28		// this will be set to non null if there is a key */

};