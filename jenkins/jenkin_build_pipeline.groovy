pipeline {
    agent none

    parameters {
        string(name: 'GIT_SHA', defaultValue: '', description: 'Git commit SHA from GitHub.')
        string(name: 'GIT_REF', defaultValue: '', description: 'Git ref or branch name from GitHub.')
        string(name: 'PR_NUMBER', defaultValue: '', description: 'Pull request number.')
        string(name: 'REPO_FULL_NAME', defaultValue: '', description: 'GitHub owner/repository name.')
        string(name: 'PR_URL', defaultValue: '', description: 'Pull request URL.')
        string(name: 'STATUS_CONTEXT', defaultValue: 'jenkins/build', description: 'GitHub commit status context.')
    }

    options {
        disableConcurrentBuilds(abortPrevious: true)
        skipDefaultCheckout()
        timestamps()
    }


    stages {
        stage('Build Context') {
            agent any
            steps {
                echo "PR #${params.PR_NUMBER} from ${params.REPO_FULL_NAME}"
                echo "Ref: ${params.GIT_REF}"
                echo "SHA: ${params.GIT_SHA}"
                echo "Status context: ${params.STATUS_CONTEXT}"
            }
        }

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
