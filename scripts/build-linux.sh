#!/bin/bash

#
#   Global variables
#
SCRIPT_FOLDER="$(cd "$(dirname "$0")" && pwd)"
source ${SCRIPT_FOLDER}/ish/common.sh

PLATFORM=linux-g++
BUILD_DIR=${PROJECT_DIR}/${BUILD_TYPE}/${TOOL_NAME}.${PLATFORM}/
LINUX_QMAKE="${QT_SDK_DIR}/gcc_64/bin/qmake"

#***************************************************************************************

print_title

prepare_libraries

new_dir ${BUILD_DIR}

print_message "Build application bundle"

pushd "${BUILD_DIR}"

    ${LINUX_QMAKE} -config ${BUILD_TYPE} ${PROJECT_DIR} VERSION="${VERSION}"

    make -j10

    cqtdeployer -bin ${APPLICATION_NAME} -qmlDir ${PROJECT_DIR}/src/qml -qmake ${LINUX_QMAKE} clear

    print_message "Copy libvs-messenger-internal.so "
    cp ${PROJECT_DIR}/ext/prebuilt/linux/release/installed/usr/local/lib/libvs-messenger-internal.so DistributionKit/lib

    print_message "Copy libvs-messenger-crypto.so "
    cp ${PROJECT_DIR}/ext/prebuilt/linux/release/installed/usr/local/lib/libvs-messenger-crypto.so DistributionKit/lib

popd
