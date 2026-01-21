@Library('jenkins-shared-library') _

pipeline {
    agent none
    options {
        parallelsAlwaysFailFast()
    }
    stages {
        stage('sentinel') {
            parallel {
                stage('clang-dev-8') {
                    agent { docker { image "${getDockerImage('clang-dev:8')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-14') {
                    agent { docker { image "${getDockerImage('clang-dev:14')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-15') {
                    agent { docker { image "${getDockerImage('clang-dev:15')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-16') {
                    agent { docker { image "${getDockerImage('clang-dev:16')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-17') {
                    agent { docker { image "${getDockerImage('clang-dev:17')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-18') {
                    agent { docker { image "${getDockerImage('clang-dev:18')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-19') {
                    agent { docker { image "${getDockerImage('clang-dev:19')}" } }
                    steps { testSentinel(report: true) }
                }
                stage('clang-dev-20') {
                    agent { docker { image "${getDockerImage('clang-dev:20')}" } }
                    steps { testSentinel(report: true) }
                }
            }
        }
    }
}
