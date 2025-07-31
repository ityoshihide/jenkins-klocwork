pipeline {
    agent any

    stages {
        stage('Klocwork inject') {
            steps {
                script {
                    sh '''
                        echo "[INFO] Add Klocwork tools to PATH"
                        export PATH=/home/ec2-user/kwbuildtools/kwbuildtools/bin:$PATH
                        echo "[INFO] Run kwinject make"
                        kwinject make
                        echo "[INFO] Check if kwinject.out exists"
                        if [ -f kwinject.out ]; then
                            echo "[SUCCESS] kwinject.out has been generated."
                        else
                            echo "[ERROR] kwinject.out was not created."
                            exit 1
                        fi
                    '''
                }
            }
        }
    }
}

