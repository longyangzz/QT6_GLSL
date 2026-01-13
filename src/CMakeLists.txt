#the old construct SUBDIRS( was substituded by ADD_SUBDIRECTORY that is to be preferred according on CMake docs.
FOREACH( 
		mylibfolder 
        BCGP
        DCGui
        DCLCMG
				GLSLViewer
    )

    ADD_SUBDIRECTORY(${mylibfolder})

ENDFOREACH()

