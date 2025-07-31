pipeline {
    agent any

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Kwinject') {
            steps {
                sh '''
                    kwinject make
                    if [ ! -f kwinject.out ]; then
                      echo "[ERROR] kwinject.out was not generated. Stopping the build."
                      exit 1
                    else
                      echo "[INFO] kwinject.out has been generated successfully."
                    fi
                '''
            }
        }
    }
}
