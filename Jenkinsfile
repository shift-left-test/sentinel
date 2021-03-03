pipeline {
    agent none
    stages {
        stage('Setup') {
            steps {
                updateGitlabCommitStatus name: 'build', state: 'pending'
            }
        }
        stage('clang-dev-9') {
            agent {
                docker { image 'cart.lge.com/swte/clang-dev:9' }
            }
	    environment {
		GTEST_OUTPUT = 'xml:result/'
	    }
            steps {
                sh './build.sh'
                sh 'gcovr --xml coverage.xml'
            }
            post {
                success {
                    junit 'test/result/*.xml'
                    cobertura coberturaReportFile: 'coverage.xml'
                }
            }
        }
        stage('clang-dev-10') {
            agent {
                docker { image 'cart.lge.com/swte/clang-dev:10' }
            }
	    environment {
		GTEST_OUTPUT = 'xml:result/'
	    }
            steps {
                sh './build.sh'
                sh 'gcovr --xml coverage.xml'
            }
            post {
                success {
                    junit 'test/result/*.xml'
                    cobertura coberturaReportFile: 'coverage.xml'
                }
            }
        }
        stage('clang-dev-11') {
            agent {
                docker { image 'cart.lge.com/swte/clang-dev:11' }
            }
	    environment {
		GTEST_OUTPUT = 'xml:result/'
	    }
            steps {
                sh './build.sh'
                sh 'gcovr --xml coverage.xml'
            }
            post {
                success {
                    junit 'test/result/*.xml'
                    cobertura coberturaReportFile: 'coverage.xml'
                }
            }
        }
    }
    post {
        failure {
            updateGitlabCommitStatus name: 'build', state: 'failed'
        }
        success {
            updateGitlabCommitStatus name: 'build', state: 'success'
        }
	aborted {
	    updateGitlabCommitStatus name: "jenkins", state: "canceled"
	}
    }
}
