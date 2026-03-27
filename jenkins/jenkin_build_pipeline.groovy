pipeline {
    agent none

    options {
        disableConcurrentBuilds(abortPrevious: true)
        timestamps()
    }

    stages {
        stage('Build Matrix') {
            matrix {
                axes {
                    axis {
                        name 'BUILD_CONFIG'
                        values 'Release', 'Debug'
                    }
                }

                agent {
                    label 'windows'
                }

                stages {
                    stage('Checkout') {
                        steps {
                            checkout scm
                        }
                    }

                    stage('Install Mono Dependency') {
                        steps {
                            powershell '''
                                if (-not (Test-Path "dep/vendor/mono")) {
                                    git clone https://github.com/Silver1713/MONO.git dep/vendor/mono
                                }
                            '''
                        }
                    }

                    stage('Configure CMake') {
                        steps {
                            powershell 'cmake -S . -B build -A x64'
                        }
                    }

                    stage('Build') {
                        steps {
                            powershell "cmake --build build --config ${env:BUILD_CONFIG} --parallel"
                        }
                    }
                }
            }
        }
    }
}
