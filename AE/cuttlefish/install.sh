#!/bin/bash

set -e

# Bootstrap, Configure, Make and Install

#
# Defining some variables
#
PROJECT_NAME=cuttlefish
PREFIX_FLAGS="--prefix=${INSTALL_PREFIX:=${PWD}/${PROJECT_NAME}-install}"
: ${NPROC:=1}

# Don't clobber our custom header template
export AUTOHEADER="echo autoheader disabled"

#
# Bootstrap
#
# if install root has been specified, add --prefix option to configure
echo "[${PROJECT_NAME}] Bootstrap..."

./bootstrap.sh

#
# Configure
#
echo "[${PROJECT_NAME}] Configure..."

COMPTREE=$PWD/compileTree
mkdir -p ${COMPTREE}

cd ${COMPTREE}

../configure ${PREFIX_FLAGS} ${CUTTLEFISH_FLAGS} $*

#
# Make
#
echo "[${PROJECT_NAME}] Make..."
make -j${NPROC}

#
# Make install
#
# if install root has been specified, perform make install
echo "[${PROJECT_NAME}] Make install... to ${INSTALL_PREFIX}"
make -j${NPROC} install

#
# Create environment setup script
#
# if install root has been specified, perform make install
CUTTLEFISH_ENV_SETUP_SCRIPT=${INSTALL_PREFIX}/bin/cuttlefish_setup_env.sh

mkdir -p `dirname ${CUTTLEFISH_ENV_SETUP_SCRIPT}`
cat > "${CUTTLEFISH_ENV_SETUP_SCRIPT}" <<EOI
# HClib environment setup
export CUTTLEFISH_ROOT='${INSTALL_PREFIX}'
EOI

cat <<EOI
[${PROJECT_NAME}] Installation complete.

${PROJECT_NAME} installed to: ${INSTALL_PREFIX}

Add the following to your .bashrc (or equivalent):
source ${CUTTLEFISH_ENV_SETUP_SCRIPT}
EOI
