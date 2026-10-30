# SYNOPSIS
#
#   AC_MSP([required])
#
# DESCRIPTION
#
# This looks for the MSP libraries. If we find them, we set the Makefile
# conditional HAVE_MSP. We as set MSP_CFLAGS and MSP_LIBS
# 
# Not finding this library might or might not indicate an error. If you
# want an error to occur if we don't find the library, then specify
# "required". Otherwise, leave it as empty and we'll just silently
# return if we don't find the library.
#
# Note that the library uses a very old (4.85) version of gcc. We have flags to
# link using the old ABI - but this should just be used in small wrapper type libraries
# (e.g., we don't want all of geocal to use the old flag, just the small swig wrappers
# we have for using the MSP library).

AC_DEFUN([AC_MSP],
[
# Guard against running twice
if test "x$done_msp" = "x"; then
AC_HANDLE_WITH_ARG_DEFAULT_NO([msp], [msp], [MSP], $1)
have_msp="no"
if test "x$want_msp" = "xyes"; then
        AC_MSG_CHECKING([for MSP library])
        succeeded=no
        if test "$ac_msp_path" != ""; then
            MSP_LIBS="-R$ac_msp_path/lib -L$ac_msp_path/lib  -ldl -R$ac_msp_path/lib -L$ac_msp_path/lib -lMSPCoordinateConversionService -lMSPCovarianceService -lMSPImagingGeometryService -lMSPMensurationService -lMSPMensurationSessionRecordService -lMSPPointExtractionService -lMSPOutputMethodService -lMSPSensorModelService -lMSPSensorSpecificService -lMSPSupportDataService -lMSPTerrainService -lMSPlunar -lMSPasdetre -lMSPCCSUtils -lMSPcoordconverter -lMSPcsisd -lMSPcsm -lMSPcsmutil -lMSPDEIUtil -lMSPgeometry -lMSPjson -lMSPlas -lMSPmath -lMSPnitf -lMSPntmtre -lMSPRsmGeneratorService -lMSPRageServiceUtils -lMSPrsmg -lMSPrsme -lMSPdei -lMSPmens -lMSPrutil -lMSPrage -lMSPRageServiceUtils -lMSPrageutilities -lMSPSScovmodel -lMSPSSrutil -lMSPutilities -lMSPmiisd -lMSPCoordinateConversionService"
            MSP_CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 -isystem $ac_msp_path/include -isystem $ac_msp_path/include/CCSUtils -isystem $ac_msp_path/include/CoordinateConversion -isystem $ac_msp_path/include/Covariance -isystem $ac_msp_path/include/PointExtraction -isystem $ac_msp_path/include/ImagingGeometry -isystem $ac_msp_path/include/Mensuration -isystem $ac_msp_path/include/MensurationSessionRecord -isystem $ac_msp_path/include/OutputMethod -isystem $ac_msp_path/include/SensorModel -isystem $ac_msp_path/include/SupportData -isystem $ac_msp_path/include/Terrain -isystem $ac_msp_path/include/common -isystem $ac_msp_path/include/common/csm -isystem $ac_msp_path/include/common/csmutil -isystem $ac_msp_path/include/common/dtcc -isystem $ac_msp_path/include/common/geometry -isystem $ac_msp_path/include/common/math -isystem $ac_msp_path/include/common/ntmtre -isystem $ac_msp_path/include/common/miisd -isystem $ac_msp_path/include/common/nitf -isystem $ac_msp_path/include/common/utilities -isystem $ac_msp_path/include/RsmGenerator"
	    MSP_PREFIX=$ac_msp_path
            succeeded=yes
        else
          for ac_path_tmp in $prefix $CONDA_PREFIX $THIRDPARTY /data/smyth/MSP/install ; do
             if test -e "$ac_path_tmp/lib/libMSPnitf.so" && test -r "$ac_path_tmp/lib/libMSPnitf.so"; then
 	     	MSP_LIBS="-R$ac_path_tmp/lib -L$ac_path_tmp/lib -ldl -lMSPCoordinateConversionService -lMSPCovarianceService -lMSPImagingGeometryService -lMSPMensurationService -lMSPMensurationSessionRecordService -lMSPPointExtractionService -lMSPOutputMethodService -lMSPSensorModelService -lMSPSensorSpecificService -lMSPSupportDataService -lMSPTerrainService -lMSPlunar -lMSPasdetre -lMSPCCSUtils -lMSPcoordconverter -lMSPcsisd -lMSPcsm -lMSPcsmutil -lMSPDEIUtil -lMSPgeometry -lMSPjson -lMSPlas -lMSPmath -lMSPnitf -lMSPntmtre -lMSPRsmGeneratorService -lMSPRageServiceUtils -lMSPrsmg -lMSPrsme -lMSPdei -lMSPmens -lMSPrutil -lMSPrage -lMSPRageServiceUtils -lMSPrageutilities -lMSPSScovmodel -lMSPSSrutil -lMSPutilities -lMSPmiisd "
		MSP_CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 -isystem $ac_path_tmp/include -isystem $ac_path_tmp/include/CCSUtils -isystem $ac_path_tmp/include/CoordinateConversion -isystem $ac_path_tmp/include/Covariance -isystem $ac_path_tmp/include/PointExtraction -isystem $ac_path_tmp/include/ImagingGeometry -isystem $ac_path_tmp/include/Mensuration -isystem $ac_path_tmp/include/MensurationSessionRecord -isystem $ac_path_tmp/include/OutputMethod -isystem $ac_path_tmp/include/SensorModel -isystem $ac_path_tmp/include/SupportData -isystem $ac_path_tmp/include/Terrain -isystem $ac_path_tmp/include/common -isystem $ac_path_tmp/include/common/csm -isystem $ac_path_tmp/include/common/csmutil -isystem $ac_path_tmp/include/common/dtcc -isystem $ac_path_tmp/include/common/geometry -isystem $ac_path_tmp/include/common/math -isystem $ac_path_tmp/include/common/ntmtre -isystem $ac_path_tmp/include/common/miisd -isystem $ac_path_tmp/include/common/nitf -isystem $ac_path_tmp/include/common/utilities -isystem $ac_path_tmp/include/RsmGenerator"
		MSP_PREFIX=$ac_path_tmp
                succeeded=yes
                break
             fi
           done
        fi
        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
		MSP_CFLAGS=""
		MSP_LIBS=""
		have_msp="no"
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(MSP_CFLAGS)
                AC_SUBST(MSP_LIBS)
                AC_SUBST(MSP_PREFIX)
                AC_DEFINE(HAVE_MSP,,[Defined if we have MSP])
                have_msp="yes"
        fi
fi
AC_SUBST(have_msp)
AM_CONDITIONAL([HAVE_MSP], [test "$have_msp" = "yes"])

AC_CHECK_FOUND([msp], [msp],[MSP],$1)

done_msp="yes"
fi
])
