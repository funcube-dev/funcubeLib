/***************************************************************************
 *  This file is part of Qthid.
 *
 *  Copyright (C) 2010  Howard Long, G6LVB
 *  CopyRight (C) 2011  Alexandru Csete, OZ9AEC
 *                      Mario Lorenz, DL5MLO
 *
 *  Qthid is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Qthid is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Qthid.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/

#ifndef FCD_H
#define FCD_H 1


#ifdef FCD
#define EXTERN
#define ASSIGN (x) =x
#else
#define EXTERN extern
#define ASSIGN(x)
#endif

#ifdef _WIN32
#define FCD_API_EXPORT __declspec(dllexport)
#define FCD_API_CALL  _stdcall
#else
#define FCD_API_EXPORT
#define FCD_API_CALL
#endif

#include <inttypes.h>
#include <stdio.h>

/** \brief FCD version enumeration. */
typedef enum {
    FCD_VERSION_NONE,/*!< No FCD detected */
    FCD_VERSION_1,  /*!< Original FCD. */
    FCD_VERSION_1_1,/*!< Bias T added in 1.1 */
    FCD_VERSION_2,  /*!< Pro+ */
    FCD_VERSION_UNK /*!< Unknown version (possibly newer than this software supports) */
} FCD_VERSION_ENUM;

/** \brief FCD mode enumeration. */
typedef enum {
    FCD_MODE_NONE,  /*!< No FCD detected. */
    FCD_MODE_BL,    /*!< FCD present in bootloader mode. */
    FCD_MODE_APP    /*!< FCD present in application mode. */
} FCD_MODE_ENUM; // The current mode of the FCD: none inserted, in bootloader mode or in normal application mode

/** \brief FCD capabilities that depend on both hardware and firmware. */
typedef struct {
    unsigned char hasBiasT;     /*!< Whether FCD has hardware bias tee (1=yes, 0=no) */
    unsigned char hasCellBlock; /*!< Whether FCD has cellular blocking. */
} FCD_CAPS_STRUCT;

#ifdef __cplusplus
extern "C" {
#endif

/* Application functions */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_VERSION_ENUM fcdGetVersion(void);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdGetMode(void);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdGetFwVerStr(char *str);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdGetCaps(FCD_CAPS_STRUCT *fcd_caps);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdGetCapsStr(char *caps_str);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdGetDeviceInfo(unsigned char *info);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdAppReset(void);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdAppSetFreqkHz(int nFreq);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdAppSetFreq(int nFreq);

EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdAppSetParam(uint8_t u8Cmd, uint8_t *pu8Data, uint8_t u8len);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdAppGetParam(uint8_t u8Cmd, uint8_t *pu8Data, uint8_t u8len);


/* Bootloader functions */
typedef FCD_API_CALL void (*fcdProgressCallback)(uint32_t start, uint32_t end, uint32_t position, int err);
#define PROG_OK	0
#define PROG_CMD_FAIL	1
#define PROG_BLK_FAIL	2
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlReset(void);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlErase(void);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlWriteFirmware(char *pc, int64_t n64Size);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlVerifyFirmware(char *pc, int64_t n64Size);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlSaveFirmware(FILE *saveFile);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlWriteFirmwareProg(char *pc, uint32_t nSize, fcdProgressCallback prog);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlVerifyFirmwareProg(char *pc, uint32_t nSize, fcdProgressCallback prog);
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_MODE_ENUM fcdBlSaveFirmwareProg(FILE *saveFile, fcdProgressCallback prog);


#ifdef __cplusplus
}
#endif

#endif // FCD_H
