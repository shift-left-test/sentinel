@Library('jenkins-shared-library') _

pipeline {
    agent none
    options {
        parallelsAlwaysFailFast()
    }
    stages {
        stage('sentinel') {
            parallel {
                stage('clang-dev-20.04-14') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-14')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-20.04-15') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-15')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-20.04-16') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-16')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-20.04-17') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-17')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-20.04-18') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-18')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-20.04-19') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-19')}" } }
                    steps { testSentinel(report: true) }
                }
                stage('clang-dev-20.04-20') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-20')}" } }
                    steps { testSentinel(report: true) }
                }
                stage('clang-dev-20.04-21') {
                    agent { docker { image "${getDockerImage('clang-dev:20.04-21')}" } }
                    steps { testSentinel(report: true) }
                }
                stage('clang-dev-24.04-22') {
                    agent { docker { image "${getDockerImage('clang-dev:24.04-22')}" } }
                    steps { testSentinel(report: true) }
                }
            }
        }
    }
}
