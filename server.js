const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')

const SLACK_WEBHOOK_URL = process.env.SLACK_WEBHOOK_URL
const slack = require('slack-notify')(SLACK_WEBHOOK_URL)

const portName = process.env.PORT_NAME || '/dev/ttyUSB0'
const baudRate = process.env.BAUD_RATE || 9600
const port = new SerialPort(portName, { baudRate: baudRate })
const lowFilamentWarnLimitGrams = (process.env.WARN_GRAM_LIMIT = 350)
const lowFilamentCriticalLimitGrams = (process.env.CRITICAL_GRAM_LIMIT = 275)
const sendAlertIntervalMin = 15
const sendInfoIntervalMin = 60

let lastSendAlertMS = 0
let lastSendInfoMS = 0

const parser = new Readline()
port.pipe(parser)

function sendSlack(message) {
  slack.alert({ text: message, fields: {} })
}

function handleAlert(alertType, grams) {
  // if we haven't sent an alert in the interval
  const nowMS = new Date().getTime()
  if (nowMS - lastSendAlertMS > sendAlertIntervalMin * 60 * 1000) {
    lastSendAlertMS = nowMS

    if (alertType === 'warn') {
      sendSlack(`WARNING: filament weight remaining is ${grams}`)
    } else if (alertType === 'critical') {
      sendSlack(`CRITICAL!! : filament weight remaining is ${grams}`)
    }
  } else if (nowMS - lastSendInfoMS > sendInfoIntervalMin * 60 * 1000) {
    sendSlack(`info : filament weight remaining is ${grams}`)
  }
}

function handleSerialInput(input) {
  console.log('got ', input)

  // be sure we're handling the output of weight grams and not some other serial message
  if (input.lastIndexOf('weight grams:') !== -1) {
    const [_, weight] = input.split(':')
    const weightFloat = parseFloat(weight)

    if (weightFloat < lowFilamentCriticalLimitGrams) {
      handleAlert('critical')
    } else if (weightFloat < lowFilamentWarnLimitGrams) {
      handleAlert('warn')
    }
  }
}

parser.on('data', handleSerialInput)
