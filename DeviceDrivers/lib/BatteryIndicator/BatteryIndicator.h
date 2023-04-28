/*****************************************************************************\
*                                                                           *
* File(s): BatteryIndicator.h and BatteryIndicator.cpp                      *
*                                                                           *
* Content: A battery indicator class that measures the voltage and          *
*          remaining capacity of a Lithium polymer (LiPo) rechargeable.     *
*                                                                           *
*                                                                           *
* Author(s): Tom Uhlmann                                                    *
*                                                                           *
*                                                                           *
* The file(s) mentioned above are provided as is under the terms of the     *
* MIT License without any warranty or guaranty to work properly.            *
* For additional license, copyright and contact/support issues see the      *
* supplied documentation.                                                   *
*                                                                           *
\****************************************************************************/
#ifndef ARDUFORGE_BATTERYINDICATOR_H
#define ARDUFORGE_BATTERYINDICATOR_H

#include <Arduino.h>
#include <inttypes.h>

namespace ArduForge{
    /**
     * \brief Measures voltage and remaining charge of various types of rechargeable batteries.
     * 
     * The voltage of connected batteries is measured at an analog pin and translated to a relative battery charge. A default rechargeable battery will provide an average voltage of about 1.2V. We will assume that a battery is fully charged (100%) at 1.3V and fully depleted (0%) at 1.05V. Typical Cut Off voltage for NiMh cells is 1.0V. Draining charge below this lower bound value may destroy the battery, so don't do that. If we define 1.05V as fully depleted, we introduce a small security margin.
     * \remarks Battery measurement is not an exact science. Remaining charge highly depends on quality and age of the batteries and can vary considerably between manufacturers. Make sure to provide the correct voltage your boards operates with in BatteryIndicatorNiMH::begin. Even if it is a 5V pin, it is unlikely that it will output 5V exactly. I measured 4.98V on an Uno and 4.71V on a Nano. These voltage differences have a considerable impact on measurement accuracy, if the wrong voltage is assumed.
     * 
     */
    class BatteryIndicator{
    public:
        enum BatteryType:uint8_t{
            TYPE_NIMH, // Nickle Metal Hydride
            TYPE_LIPO, ///< Lithium Polymer
        }; //BatteryType

        /**
         * \brief Constructor
         */
        BatteryIndicator(void);

        /**
         * \brief Destructor
         */
        ~BatteryIndicator(void);

        /**
         * \brief Sets up the battery measurement.
         * 
         * \param[in] BatterPin Analog pin for the batterie's anode
         * \param[in] BatteryCount Number of batteries that are connected in series.
         * \param[in] R1 Value of first resistor in voltage divider circuit. Set 0 if you don't use one.
         * \param[in] R2 Value of second resistor in voltage divider circuit. Set 0 if you don't use one.
         * @param[in] Type Type of battery which is measured.
         * \param[in] ReferenceVoltage Absolute voltage the board provides to its pins.
         */
        void begin(uint8_t BatteryPin, uint8_t BatteryCount = 1, uint32_t R1 = 47000, uint32_t R2 = 22000, BatteryType Type = TYPE_LIPO, float ReferenceVoltage = 4.98f);

        /**
         * \brief Ends communication with the battery.
         */
        void end(void);

        /**
         * \brief Measures the voltage, the battery cell provides.
         * 
         * \return Voltage the connected batteries provide.
         */
        float read(void);

        /**
         * \brief Computes the remaining charge of the battery in percent.
         * 
         * \return Remaining charge of the batteries in percent [0, 100].
         */
        uint8_t charge(void);

        /**
         * @brief Returns battery type.
         * @return Battery type.
         */
        BatteryType type(void)const;

    private:       
        float m_RefVoltage;     ///< Absolute voltage the board operates with.
        float m_R1;             ///< Resistor value 1.
        float m_R2;             ///< Resistor value 2.
        float m_VoltageMin;     ///< Minimum voltage of a single cell (dependent on type)
        float m_VoltageMax;     ///< Maximum voltage of a single cell (dependent on type)
        uint8_t m_BatteryPin;   ///< Analog pin the battery is connected to.
        uint8_t m_BatteryCount; ///< Number of batteries that are connected in series.
        BatteryType m_Type;     ///< Battery type.

    };//BatteryIndicator
}


#endif 