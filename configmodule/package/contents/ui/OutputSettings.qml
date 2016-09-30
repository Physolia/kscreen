/*
    Copyright Sebastian Kügler <sebas@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

import QtQuick 2.1
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrols 2.0
import org.kde.kscreen 2.0

GridLayout {

    property KScreenOutput output: (qmlOutput === null) ? null : qmlOutput.output
    property QMLOutput qmlOutput: null

    focus: true
    columns: 2
    rowSpacing: units.largeSpacing

    Layout.maximumHeight: childrenRect.height

    onOutputChanged: {
        if (output === null) return;
        print("Output is now: " + outputLabel.text);
        print(" Modes: " + qmlOutput.modes.length);
        print(" Modes: " + qmlOutput.modeSizes.length);
    }

    PlasmaExtras.Heading {
        id: outputLabel
        level: 2
        text: {
            if (output === null) return "";
            if (output.type == KScreenOutput.Panel) {
                return i18n("Laptop Screen");
            }
            return output.edid.vendor + " " + output.edid.name + " (" + output.name + ")"
        }
        Layout.columnSpan: 2
    }

    Label {
        Layout.fillWidth: true
        Layout.columnSpan: 2
        visible: root.perOutputScaling
        text: i18nc("value label of scaling slider", "%1x", scalingSlider.value)
        horizontalAlignment: Text.AlignRight
    }

    Label {
        visible: root.perOutputScaling
        text: i18n("Scaling factor:")
        Layout.alignment: Qt.AlignTop | Qt.AlignRight
    }

    Slider {
        id: scalingSlider
        visible: root.perOutputScaling
        Layout.fillWidth: true
        minimumValue: 1
        maximumValue: 3
        stepSize: .2
        tickmarksEnabled: true
    }

    Label {
        Layout.fillWidth: true
        Layout.columnSpan: 2
        text: qmlOutput.selectedMode ? qmlOutput.selectedMode.size.width + "x" + qmlOutput.selectedMode.size.height + "@" + qmlOutput.selectedMode.refreshRate.toFixed(2) : ""
        horizontalAlignment: Text.AlignRight
    }

    Label {
        text: i18n("Resolution and refresh rate:")
        Layout.alignment: Qt.AlignTop | Qt.AlignRight
    }

    ColumnLayout {
        id: modecol
        property KScreenMode selectedMode: null

        Slider {
            id: resolutionSlider
            Layout.fillWidth: true
            tickmarksEnabled: true
            minimumValue: 0
            maximumValue: (qmlOutput === null) ? 1 : qmlOutput.modeSizes.length - 2
            stepSize: 1
            onValueChanged: {
                qmlOutput.setSelectedSize(value)
            }
        }

        RowLayout {
            opacity: (output != null && qmlOutput.refreshRates.length > 1) ? 1.0 : 0.6
            Behavior on opacity { NumberAnimation { duration: units.longDuration } }
            CheckBox {
                id: refreshAutoCheck
                checked: true
                text: i18n("Auto");
            }
            Slider {
                Layout.fillWidth: true
                tickmarksEnabled: true
                minimumValue: 0
                enabled: !refreshAutoCheck.checked
                maximumValue: (qmlOutput === null || qmlOutput.refreshRates.length <= 1) ? 1 : qmlOutput.refreshRates.length - 1
                stepSize: 1
                onValueChanged: qmlOutput.setSelectedRefreshRate(value)
            }
        }
    }
    Item {
        Layout.preferredHeight: units.largeSpacing
    }
}
