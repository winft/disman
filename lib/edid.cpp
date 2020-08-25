/*************************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti <dantti12@gmail.com>                     *
 *             (C) 2012 - 2014 by Daniel Vrátil <dvratil@redhat.com>                 *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Lesser General Public                       *
 *  License as published by the Free Software Foundation; either                     *
 *  version 2.1 of the License, or (at your option) any later version.               *
 *                                                                                   *
 *  This library is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 *  Lesser General Public License for more details.                                  *
 *                                                                                   *
 *  You should have received a copy of the GNU Lesser General Public                 *
 *  License along with this library; if not, write to the Free Software              *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA       *
 *************************************************************************************/
#include "edid.h"
#include "disman_debug_edid.h"

#include <math.h>

#include <QCryptographicHash>
#include <QFile>
#include <QStringBuilder>
#include <QStringList>

#define GCM_EDID_OFFSET_PNPID 0x08
#define GCM_EDID_OFFSET_SERIAL 0x0c
#define GCM_EDID_OFFSET_SIZE 0x15
#define GCM_EDID_OFFSET_GAMMA 0x17
#define GCM_EDID_OFFSET_DATA_BLOCKS 0x36
#define GCM_EDID_OFFSET_LAST_BLOCK 0x6c
#define GCM_EDID_OFFSET_EXTENSION_BLOCK_COUNT 0x7e

#define GCM_DESCRIPTOR_DISPLAY_PRODUCT_NAME 0xfc
#define GCM_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER 0xff
#define GCM_DESCRIPTOR_COLOR_MANAGEMENT_DATA 0xf9
#define GCM_DESCRIPTOR_ALPHANUMERIC_DATA_STRING 0xfe
#define GCM_DESCRIPTOR_COLOR_POINT 0xfb

#define PNP_IDS "/usr/share/hwdata/pnp.ids"

using namespace Disman;

class Edid::Private
{
public:
    Private()
        : valid(false)
        , width(0)
        , height(0)
        , gamma(0)
    {
    }

    Private(const Private& other)
        : valid(other.valid)
        , monitorName(other.monitorName)
        , vendorName(other.vendorName)
        , serialNumber(other.serialNumber)
        , eisaId(other.eisaId)
        , checksum(other.checksum)
        , pnpId(other.pnpId)
        , width(other.width)
        , height(other.height)
        , gamma(other.gamma)
        , red(other.red)
        , green(other.green)
        , blue(other.blue)
        , white(other.white)
    {
    }

    bool parse(const QByteArray& data);
    int edidGetBit(int in, int bit) const;
    int edidGetBits(int in, int begin, int end) const;
    float edidDecodeFraction(int high, int low) const;
    std::string edidParseString(const quint8* data) const;

    bool valid;
    std::string monitorName;
    std::string vendorName;
    std::string serialNumber;
    std::string eisaId;
    std::string checksum;
    std::string pnpId;
    uint width;
    uint height;
    qreal gamma;
    QQuaternion red;
    QQuaternion green;
    QQuaternion blue;
    QQuaternion white;
};

Edid::Edid()
    : d(new Private())
{
}

Edid::Edid(const QByteArray& data)
    : d(new Private())
{
    d->parse(data);
}

Edid::Edid(Edid::Private* dd)
    : d(new Private(*dd))
{
}

Edid::~Edid()
{
    delete d;
}

Edid* Edid::clone() const
{
    return new Edid(new Private(*d));
}

bool Edid::isValid() const
{
    return d->valid;
}

std::string Edid::deviceId(const std::string& fallbackName) const
{
    std::string id = "xrandr";
    // if no info was added check if the fallbacName is provided
    if (!vendor().size() && !name().size() && !serial().size()) {
        if (fallbackName.size()) {
            id.append('-' + fallbackName);
        } else {
            // all info we have are empty strings
            id.append("-unknown");
        }
    } else if (d->valid) {
        if (vendor().size()) {
            id.append('-' + vendor());
        }
        if (name().size()) {
            id.append('-' + name());
        }
        if (serial().size()) {
            id.append('-' + serial());
        }
    }

    return id;
}

std::string Edid::name() const
{
    if (d->valid) {
        return d->monitorName;
    }
    return std::string();
}

std::string Edid::vendor() const
{
    if (d->valid) {
        return d->vendorName;
    }
    return std::string();
}

std::string Edid::serial() const
{
    if (d->valid) {
        return d->serialNumber;
    }
    return std::string();
}

std::string Edid::eisaId() const
{
    if (d->valid) {
        return d->eisaId;
    }
    return std::string();
}

std::string Edid::hash() const
{
    if (d->valid) {
        return d->checksum;
    }
    return std::string();
}

std::string Edid::pnpId() const
{
    if (d->valid) {
        return d->pnpId;
    }
    return std::string();
}

uint Edid::width() const
{
    return d->width;
}

uint Edid::height() const
{
    return d->height;
}

qreal Edid::gamma() const
{
    return d->gamma;
}

QQuaternion Edid::red() const
{
    return d->red;
}

QQuaternion Edid::green() const
{
    return d->green;
}

QQuaternion Edid::blue() const
{
    return d->blue;
}

QQuaternion Edid::white() const
{
    return d->white;
}

bool Edid::Private::parse(const QByteArray& rawData)
{
    const quint8* data = reinterpret_cast<const quint8*>(rawData.constData());
    int length = rawData.length();

    /* check header */
    if (length < 128) {
        if (length > 0) {
            qCWarning(DISMAN_EDID) << "Invalid EDID length (" << length << " bytes)";
        }
        valid = false;
        return valid;
    }
    if (data[0] != 0x00 || data[1] != 0xff) {
        qCWarning(DISMAN_EDID) << "Failed to parse EDID header";
        valid = false;
        return valid;
    }

    /* decode the PNP ID from three 5 bit words packed into 2 bytes
     * /--08--\/--09--\
     * 7654321076543210
     * |\---/\---/\---/
     * R  C1   C2   C3 */
    pnpId.resize(3);
    pnpId[0] = 'A' + ((data[GCM_EDID_OFFSET_PNPID + 0] & 0x7c) / 4) - 1;
    pnpId[1] = 'A' + ((data[GCM_EDID_OFFSET_PNPID + 0] & 0x3) * 8)
        + ((data[GCM_EDID_OFFSET_PNPID + 1] & 0xe0) / 32) - 1;
    pnpId[2] = 'A' + (data[GCM_EDID_OFFSET_PNPID + 1] & 0x1f) - 1;
    pnpId.resize(strlen(pnpId.data()));

    // load the PNP_IDS file and load the vendor name
    if (pnpId.size()) {
        QFile pnpIds(QStringLiteral(PNP_IDS));
        if (pnpIds.open(QIODevice::ReadOnly)) {
            while (!pnpIds.atEnd()) {
                QString line = QString::fromUtf8(pnpIds.readLine());
                if (line.startsWith(QString::fromStdString(pnpId))) {
                    QStringList parts = line.split(QLatin1Char('\t'));
                    if (parts.size() == 2) {
                        vendorName = line.split(QLatin1Char('\t')).at(1).simplified().toStdString();
                    }
                    break;
                }
            }
        }
    }

    /* maybe there isn't a ASCII serial number descriptor, so use this instead */
    auto serial = static_cast<uint32_t>(data[GCM_EDID_OFFSET_SERIAL + 0]);
    serial += static_cast<uint32_t>(data[GCM_EDID_OFFSET_SERIAL + 1] * 0x100);
    serial += static_cast<uint32_t>(data[GCM_EDID_OFFSET_SERIAL + 2] * 0x10000);
    serial += static_cast<uint32_t>(data[GCM_EDID_OFFSET_SERIAL + 3] * 0x1000000);
    if (serial > 0) {
        serialNumber = std::to_string(serial);
    }

    /* get the size */
    width = data[GCM_EDID_OFFSET_SIZE + 0];
    height = data[GCM_EDID_OFFSET_SIZE + 1];

    /* we don't care about aspect */
    if (width == 0 || height == 0) {
        width = 0;
        height = 0;
    }

    /* get gamma */
    if (data[GCM_EDID_OFFSET_GAMMA] == 0xff) {
        gamma = 1.0;
    } else {
        gamma = data[GCM_EDID_OFFSET_GAMMA] / 100.0 + 1.0;
    }

    /* get color red */
    red.setX(edidDecodeFraction(data[0x1b], edidGetBits(data[0x19], 6, 7)));
    red.setY(edidDecodeFraction(data[0x1c], edidGetBits(data[0x19], 5, 4)));

    /* get color green */
    green.setX(edidDecodeFraction(data[0x1d], edidGetBits(data[0x19], 2, 3)));
    green.setY(edidDecodeFraction(data[0x1e], edidGetBits(data[0x19], 0, 1)));

    /* get color blue */
    blue.setX(edidDecodeFraction(data[0x1f], edidGetBits(data[0x1a], 6, 7)));
    blue.setY(edidDecodeFraction(data[0x20], edidGetBits(data[0x1a], 4, 5)));

    /* get color white */
    white.setX(edidDecodeFraction(data[0x21], edidGetBits(data[0x1a], 2, 3)));
    white.setY(edidDecodeFraction(data[0x22], edidGetBits(data[0x1a], 0, 1)));

    /* parse EDID data */
    for (uint i = GCM_EDID_OFFSET_DATA_BLOCKS; i <= GCM_EDID_OFFSET_LAST_BLOCK; i += 18) {
        /* ignore pixel clock data */
        if (data[i] != 0) {
            continue;
        }
        if (data[i + 2] != 0) {
            continue;
        }

        /* any useful blocks? */
        if (data[i + 3] == GCM_DESCRIPTOR_DISPLAY_PRODUCT_NAME) {
            monitorName = edidParseString(&data[i + 5]);
        } else if (data[i + 3] == GCM_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER) {
            serialNumber = edidParseString(&data[i + 5]);
        } else if (data[i + 3] == GCM_DESCRIPTOR_COLOR_MANAGEMENT_DATA) {
            qCWarning(DISMAN_EDID) << "failing to parse color management data";
        } else if (data[i + 3] == GCM_DESCRIPTOR_ALPHANUMERIC_DATA_STRING) {
            eisaId = edidParseString(&data[i + 5]);
        } else if (data[i + 3] == GCM_DESCRIPTOR_COLOR_POINT) {
            if (data[i + 3 + 9] != 0xff) {
                /* extended EDID block(1) which contains
                 * a better gamma value */
                gamma = (data[i + 3 + 9] / 100.0) + 1;
            }
            if (data[i + 3 + 14] != 0xff) {
                /* extended EDID block(2) which contains
                 * a better gamma value */
                gamma = (data[i + 3 + 9] / 100.0) + 1;
            }
        }
    }

    // calculate checksum
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(reinterpret_cast<const char*>(data), length);
    checksum = QString::fromLatin1(hash.result().toHex()).toStdString();

    valid = true;
    return valid;
}

int Edid::Private::edidGetBit(int in, int bit) const
{
    return (in & (1 << bit)) >> bit;
}

int Edid::Private::edidGetBits(int in, int begin, int end) const
{
    int mask = (1 << (end - begin + 1)) - 1;

    return (in >> begin) & mask;
}

float Edid::Private::edidDecodeFraction(int high, int low) const
{
    float result = 0.0;

    high = (high << 2) | low;
    for (int i = 0; i < 10; ++i) {
        result += edidGetBit(high, i) * pow(2, i - 10);
    }
    return result;
}

std::string Edid::Private::edidParseString(const quint8* data) const
{
    /* this is always 13 bytes, but we can't guarantee it's null
     * terminated or not junk. */
    auto text = QString::fromLatin1(reinterpret_cast<const char*>(data), 13).simplified();

    for (int i = 0; i < text.size(); ++i) {
        if (!text.at(i).isPrint()) {
            text[i] = '-';
        }
    }
    return text.toStdString();
}
